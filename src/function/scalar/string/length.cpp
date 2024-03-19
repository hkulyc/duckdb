#include "duckdb/function/scalar/string_functions.hpp"
#include "duckdb/common/types/bit.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"

#include "duckdb/planner/expression/bound_parameter_expression.hpp"
#include "utf8proc.hpp"

namespace duckdb {

// length returns the number of unicode codepoints
struct StringLengthOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return LengthFun::Length<TA, TR>(input);
	}
};

struct GraphemeCountOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return LengthFun::GraphemeCount<TA, TR>(input);
	}
};

// strlen returns the size in bytes
struct StrLenOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return input.GetSize();
	}
};

struct OctetLenOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return Bit::OctetLength(input);
	}
};

// bitlen returns the size in bits
struct BitLenOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return 8 * input.GetSize();
	}
};

// bitstringlen returns the amount of bits in a bitstring
struct BitStringLenOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return Bit::BitLength(input);
	}
};

static unique_ptr<BaseStatistics> LengthPropagateStats(ClientContext &context, FunctionStatisticsInput &input) {
	auto &child_stats = input.child_stats;
	auto &expr = input.expr;
	D_ASSERT(child_stats.size() == 1);
	// can only propagate stats if the children have stats
	if (!StringStats::CanContainUnicode(child_stats[0])) {
		expr.function.function = ScalarFunction::UnaryFunction<string_t, int64_t, StrLenOperator>;
	}
	return nullptr;
}

//------------------------------------------------------------------
// ARRAY / LIST LENGTH
//------------------------------------------------------------------

static void ListLengthFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &input = args.data[0];
	D_ASSERT(input.GetType().id() == LogicalTypeId::LIST);
	UnaryExecutor::Execute<list_entry_t, int64_t>(input, result, args.size(),
	                                              [](list_entry_t input) { return input.length; });
	if (args.AllConstant()) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

static void ArrayLengthFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &input = args.data[0];
	// If the input is an array, the length is constant
	result.SetVectorType(VectorType::CONSTANT_VECTOR);
	ConstantVector::GetData<int64_t>(result)[0] = static_cast<int64_t>(ArrayType::GetSize(input.GetType()));
}

static unique_ptr<FunctionData> ArrayOrListLengthBind(ClientContext &context, ScalarFunction &bound_function,
                                                      vector<unique_ptr<Expression>> &arguments) {
	if (arguments[0]->HasParameter()) {
		throw ParameterNotResolvedException();
	}
	if (arguments[0]->return_type.id() == LogicalTypeId::ARRAY) {
		bound_function.function = ArrayLengthFunction;
	} else if (arguments[0]->return_type.id() == LogicalTypeId::LIST) {
		bound_function.function = ListLengthFunction;
	} else {
		// Unreachable
		throw BinderException("length can only be used on arrays or lists");
	}
	bound_function.arguments[0] = arguments[0]->return_type;
	return nullptr;
}

//------------------------------------------------------------------
// ARRAY / LIST WITH DIMENSION
//------------------------------------------------------------------
static void ListLengthBinaryFunction(DataChunk &args, ExpressionState &, Vector &result) {
	auto type = args.data[0].GetType();
	auto &input = args.data[0];
	auto &dimension = args.data[1];
	BinaryExecutor::Execute<list_entry_t, int64_t, int64_t>(
	    input, dimension, result, args.size(), [](list_entry_t input, int64_t dimension) {
		    if (dimension != 1) {
			    throw NotImplementedException("array_length for lists with dimensions other than 1 not implemented");
		    }
		    return input.length;
	    });
	if (args.AllConstant()) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

struct ArrayLengthBinaryFunctionData : public FunctionData {
	vector<int64_t> dimensions;

	unique_ptr<FunctionData> Copy() const override {
		auto copy = make_uniq<ArrayLengthBinaryFunctionData>();
		copy->dimensions = dimensions;
		return std::move(copy);
	}

	bool Equals(const FunctionData &other) const override {
		auto &other_data = other.Cast<const ArrayLengthBinaryFunctionData>();
		return dimensions == other_data.dimensions;
	}
};

static void ArrayLengthBinaryFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto type = args.data[0].GetType();
	auto &dimension = args.data[1];

	auto &expr = state.expr.Cast<BoundFunctionExpression>();
	auto &data = expr.bind_info->Cast<ArrayLengthBinaryFunctionData>();
	auto &dimensions = data.dimensions;
	auto max_dimension = static_cast<int64_t>(dimensions.size());

	UnaryExecutor::Execute<int64_t, int64_t>(dimension, result, args.size(), [&](int64_t dimension) {
		if (dimension < 1 || dimension > max_dimension) {
			throw OutOfRangeException(StringUtil::Format(
			    "array_length dimension '%lld' out of range (min: '1', max: '%lld')", dimension, max_dimension));
		}
		return dimensions[dimension - 1];
	});

	if (args.AllConstant()) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

static unique_ptr<FunctionData> ArrayOrListLengthBinaryBind(ClientContext &context, ScalarFunction &bound_function,
                                                            vector<unique_ptr<Expression>> &arguments) {
	if (arguments[0]->HasParameter()) {
		throw ParameterNotResolvedException();
	}
	auto type = arguments[0]->return_type;
	if (type.id() == LogicalTypeId::ARRAY) {
		bound_function.arguments[0] = type;
		bound_function.function = ArrayLengthBinaryFunction;

		// If the input is an array, the dimensions are constant, so we can calculate them at bind time
		vector<int64_t> dimensions;
		while (true) {
			if (type.id() == LogicalTypeId::ARRAY) {
				dimensions.push_back(ArrayType::GetSize(type));
				type = ArrayType::GetChildType(type);
			} else {
				break;
			}
		}
		auto data = make_uniq<ArrayLengthBinaryFunctionData>();
		data->dimensions = dimensions;
		return std::move(data);

	} else if (type.id() == LogicalTypeId::LIST) {
		bound_function.function = ListLengthBinaryFunction;
		bound_function.arguments[0] = type;
		return nullptr;
	} else {
		// Unreachable
		throw BinderException("array_length can only be used on arrays or lists");
	}
}

void LengthFun::RegisterFunction(BuiltinFunctions &set) {
	// udf_todo
	ScalarFunction array_length_unary =
	    ScalarFunction({LogicalType::LIST(LogicalType::ANY)}, LogicalType::BIGINT,
	                   nullptr, ScalarFunctionInfo("ArrayLengthOperator::Operation", {"list_entry_t", "int64_t"}), ArrayOrListLengthBind);
	ScalarFunctionSet length("length");
	length.AddFunction(ScalarFunction({LogicalType::VARCHAR}, LogicalType::BIGINT,
	                                  ScalarFunction::UnaryFunction<string_t, int64_t, StringLengthOperator>, ScalarFunctionInfo("StringLengthOperator::Operation", {"string_t", "int64_t"}), nullptr,
	                                  nullptr, LengthPropagateStats));
	length.AddFunction(ScalarFunction({LogicalType::BIT}, LogicalType::BIGINT,
	                                  ScalarFunction::UnaryFunction<string_t, int64_t, BitStringLenOperator>, ScalarFunctionInfo("BitStringLenOperator::Operation", {"string_t", "int64_t"})));
	length.AddFunction(array_length_unary);
	set.AddFunction(length);
	length.name = "len";
	set.AddFunction(length);

	ScalarFunctionSet length_grapheme("length_grapheme");
	length_grapheme.AddFunction(ScalarFunction({LogicalType::VARCHAR}, LogicalType::BIGINT,
	                                           ScalarFunction::UnaryFunction<string_t, int64_t, GraphemeCountOperator>, ScalarFunctionInfo("GraphemeCountOperator::Operation", {"string_t", "int64_t"}),
	                                           nullptr, nullptr, LengthPropagateStats));
	set.AddFunction(length_grapheme);

	ScalarFunctionSet array_length("array_length");
	array_length.AddFunction(array_length_unary);
	// udf_todo
	array_length.AddFunction(ScalarFunction(
	    {LogicalType::LIST(LogicalType::ANY), LogicalType::BIGINT}, LogicalType::BIGINT,
	    nullptr, ScalarFunctionInfo("ArrayLengthBinaryOperator::Operation", {"list_entry_t", "int64_t", "int64_t"}),
		ArrayOrListLengthBinaryBind));
	set.AddFunction(array_length);

	set.AddFunction(ScalarFunction("strlen", {LogicalType::VARCHAR}, LogicalType::BIGINT,
	                               ScalarFunction::UnaryFunction<string_t, int64_t, StrLenOperator>, ScalarFunctionInfo("StrLenOperator::Operation", {"string_t", "int64_t"})));
	ScalarFunctionSet bit_length("bit_length");
	bit_length.AddFunction(ScalarFunction({LogicalType::VARCHAR}, LogicalType::BIGINT,
	                                      ScalarFunction::UnaryFunction<string_t, int64_t, BitLenOperator>, ScalarFunctionInfo("BitLenOperator::Operation", {"string_t", "int64_t"})));
	bit_length.AddFunction(ScalarFunction({LogicalType::BIT}, LogicalType::BIGINT,
	                                      ScalarFunction::UnaryFunction<string_t, int64_t, BitStringLenOperator>, ScalarFunctionInfo("BitStringLenOperator::Operation", {"string_t", "int64_t"})));
	set.AddFunction(bit_length);
	// length for BLOB type
	ScalarFunctionSet octet_length("octet_length");
	octet_length.AddFunction(ScalarFunction({LogicalType::BLOB}, LogicalType::BIGINT,
	                                        ScalarFunction::UnaryFunction<string_t, int64_t, StrLenOperator>, ScalarFunctionInfo("StrLenOperator::Operation", {"string_t", "int64_t"})));
	octet_length.AddFunction(ScalarFunction({LogicalType::BIT}, LogicalType::BIGINT,
	                                        ScalarFunction::UnaryFunction<string_t, int64_t, OctetLenOperator>, ScalarFunctionInfo("OctetLenOperator::Operation", {"string_t", "int64_t"})));
	set.AddFunction(octet_length);
}

} // namespace duckdb
