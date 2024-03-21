#include "duckdb/function/scalar_function.hpp"

namespace duckdb {

FunctionLocalState::~FunctionLocalState() {
}

ScalarFunction::ScalarFunction(string name, vector<LogicalType> arguments, LogicalType return_type,
                               scalar_function_t function, bind_scalar_function_t bind,
                               dependency_function_t dependency, function_statistics_t statistics,
                               init_local_state_t init_local_state, LogicalType varargs,
                               FunctionSideEffects side_effects, FunctionNullHandling null_handling,
                               bind_lambda_function_t bind_lambda)
    : BaseScalarFunction(std::move(name), std::move(arguments), std::move(return_type), side_effects,
                         std::move(varargs), null_handling),
      function(std::move(function)), bind(bind), init_local_state(init_local_state), dependency(dependency),
      statistics(statistics), bind_lambda(bind_lambda), serialize(nullptr), deserialize(nullptr) {
}

ScalarFunction::ScalarFunction(string name, vector<LogicalType> arguments, LogicalType return_type,
                               scalar_function_t function, ScalarFunctionInfo &&function_info, bind_scalar_function_t bind,
                               dependency_function_t dependency, function_statistics_t statistics,
                               init_local_state_t init_local_state, LogicalType varargs,
                               FunctionSideEffects side_effects, FunctionNullHandling null_handling,
							   bind_lambda_function_t bind_lambda)
    : BaseScalarFunction(std::move(name), std::move(arguments), std::move(return_type), side_effects,
                         std::move(varargs), null_handling),
      function(std::move(function)), function_info(std::move(function_info)), bind(bind), init_local_state(init_local_state), dependency(dependency),
      statistics(statistics), bind_lambda(bind_lambda), serialize(nullptr), deserialize(nullptr) {
	has_scalar_function_info = true;
}

ScalarFunction::ScalarFunction(vector<LogicalType> arguments, LogicalType return_type, scalar_function_t function,
                               bind_scalar_function_t bind, dependency_function_t dependency,
                               function_statistics_t statistics, init_local_state_t init_local_state,
                               LogicalType varargs, FunctionSideEffects side_effects,
                               FunctionNullHandling null_handling, bind_lambda_function_t bind_lambda)
    : ScalarFunction(string(), std::move(arguments), std::move(return_type), std::move(function), bind, dependency,
                     statistics, init_local_state, std::move(varargs), side_effects, null_handling, bind_lambda) {
}

ScalarFunction::ScalarFunction(vector<LogicalType> arguments, LogicalType return_type, scalar_function_t function,
                               ScalarFunctionInfo &&function_info, bind_scalar_function_t bind, dependency_function_t dependency,
                               function_statistics_t statistics, init_local_state_t init_local_state,
                               LogicalType varargs, FunctionSideEffects side_effects,
                               FunctionNullHandling null_handling, bind_lambda_function_t bind_lambda)
    : ScalarFunction(string(), std::move(arguments), std::move(return_type), std::move(function), std::move(function_info), bind, dependency,
                     statistics, init_local_state, std::move(varargs), side_effects, null_handling, bind_lambda) {
}

bool ScalarFunction::operator==(const ScalarFunction &rhs) const {
	return name == rhs.name && arguments == rhs.arguments && return_type == rhs.return_type && varargs == rhs.varargs &&
	       bind == rhs.bind && dependency == rhs.dependency && statistics == rhs.statistics &&
	       bind_lambda == rhs.bind_lambda;
}

bool ScalarFunction::operator!=(const ScalarFunction &rhs) const {
	return !(*this == rhs);
}

bool ScalarFunction::Equal(const ScalarFunction &rhs) const {
	// number of types
	if (this->arguments.size() != rhs.arguments.size()) {
		return false;
	}
	// argument types
	for (idx_t i = 0; i < this->arguments.size(); ++i) {
		if (this->arguments[i] != rhs.arguments[i]) {
			return false;
		}
	}
	// return type
	if (this->return_type != rhs.return_type) {
		return false;
	}
	// varargs
	if (this->varargs != rhs.varargs) {
		return false;
	}

	return true; // they are equal
}

void ScalarFunction::NopFunction(DataChunk &input, ExpressionState &state, Vector &result) {
	D_ASSERT(input.ColumnCount() >= 1);
	result.Reference(input.data[0]);
}

std::string ScalarFunctionInfo::DecimalTypeToCppType(int width, int scale){
      if(width >= 1 and width <= 4){
            return "int16_t";
      }
      else if(width >= 5 and width <= 9){
            return "int32_t";
      }
      else if(width >= 10 and width <= 18){
            return "int64_t";
      }
      else if(width >= 19 and width <= 38){
            return "hugeint_t";
      }
      else{
            throw std::runtime_error("DECIMAL type width must be between 1 and 38");
      }
}

std::string ScalarFunctionInfo::LogicalTypeToCppType(const LogicalType &logical_type) {
      switch (logical_type.GetInternalType()) {
      case PhysicalType::BOOL:
            return "bool";
      case PhysicalType::INT8:
            return "int8_t";
      case PhysicalType::INT16:
            return "int16_t";
      case PhysicalType::INT32:
            return "int32_t";
      case PhysicalType::INT64:
            return "int64_t";
      case PhysicalType::UINT8:
            return "uint8_t";
      case PhysicalType::UINT16:
            return "uint16_t";
      case PhysicalType::UINT32:
            return "uint32_t";
      case PhysicalType::UINT64:
            return "uint64_t";
      case PhysicalType::INT128:
            return "hugeint_t";
      case PhysicalType::FLOAT:
            return "float";
      case PhysicalType::DOUBLE:
            return "double";
      case PhysicalType::VARCHAR:
            return "string_t";
      // case PhysicalType::DECIMAL:
      //       return DecimalTypeToCppType(DecimalType::GetWidth(logical_type), DecimalType::GetScale(logical_type));
      default:
            printf("Unimplemented type for C++ code generation\n");
            return "UNIMPLEMENTED";

      }
}

std::string ScalarFunctionInfo::PhysicalTypeIdToCppType(PhysicalType type_id){
      switch (type_id) {
      case PhysicalType::BOOL:
            return "bool";
      case PhysicalType::INT8:
            return "int8_t";
      case PhysicalType::INT16:
            return "int16_t";
      case PhysicalType::INT32:
            return "int32_t";
      case PhysicalType::INT64:
            return "int64_t";
      case PhysicalType::UINT8:
            return "uint8_t";
      case PhysicalType::UINT16:
            return "uint16_t";
      case PhysicalType::UINT32:
            return "uint32_t";
      case PhysicalType::UINT64:
            return "uint64_t";
      case PhysicalType::INT128:
            return "hugeint_t";
      case PhysicalType::FLOAT:
            return "float";
      case PhysicalType::DOUBLE:
            return "double";
      case PhysicalType::VARCHAR:
            return "string_t";
      // case PhysicalType::STRUCT:
      //       return "struct_t";
      // case PhysicalType::LIST:
      //       return "list_entry_t";
      // case PhysicalType::MAP:
      //       return "map_t";
      default:
            printf("Unimplemented type for C++ code generation\n");
            return "UNIMPLEMENTED";
      }
}

// TranspilerScalarFunction::TranspilerScalarFunction(string name, vector<LogicalType> arguments, LogicalType return_type,
// 	                          scalar_function_t function, ScalarFunctionInfo &&function_info, bind_scalar_function_t bind,
// 	                          dependency_function_t dependency, function_statistics_t statistics,
// 	                          init_local_state_t init_local_state,
// 	                          LogicalType varargs, FunctionSideEffects side_effects,
// 	                          FunctionNullHandling null_handling)
// 	: ScalarFunction(std::move(name), std::move(arguments), std::move(return_type), std::move(function), bind, dependency,
// 		statistics, init_local_state, std::move(varargs), side_effects, null_handling), function_info(std::move(function_info)) {}

// TranspilerScalarFunction::TranspilerScalarFunction(vector<LogicalType> arguments, LogicalType return_type, scalar_function_t function, ScalarFunctionInfo &&function_info, 
// 	                          bind_scalar_function_t bind,
// 	                          dependency_function_t dependency, function_statistics_t statistics,
// 	                          init_local_state_t init_local_state,
// 	                          LogicalType varargs, FunctionSideEffects side_effects,
// 	                          FunctionNullHandling null_handling)
// 	: ScalarFunction(std::move(arguments), std::move(return_type), std::move(function), bind, dependency,
// 		statistics, init_local_state, std::move(varargs), side_effects, null_handling), function_info(std::move(function_info)) {}

} // namespace duckdb
