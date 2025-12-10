#include "intermediate.hpp"

#include <cmath>
#include <cstring>

#include "common/utils.hpp"

Value::Value(Token const& token)
: Expression{token.at}
{
	// we are cheating a bit in this function, as c++ does not fully support 128bit yet
	// so we just go up to 64 bit ints, treating 128 bit as reserved.
	// for floats we'll just assume double/float64.
	pos = token.at;
	rep = token.token;
	if (token.type == TokenType::TLiteralString) {
		kind = ValueKind::STRING;
	} else if (token.type == TokenType::TLiteralInteger) {
		if (token.token.starts_with("-")) {
			int base{10};
			auto enc = token.token;
			if (enc.starts_with("-0x")) {
				base = 16;
				enc  = "-" + enc.substr(3);
			} else if (enc.starts_with("-0o")) {
				base = 8;
				enc  = "-" + enc.substr(3);
			} else if (enc.starts_with("-0b")) {
				base = 2;
				enc  = "-" + enc.substr(3);
			}
			int64_t val;
			auto [ptr, err] = std::from_chars(enc.data(), enc.data() + enc.length(), val, base);
			if (err != std::errc{}) {
				throw file_error("Unable to parse integer literal", pos);
			}
			if (val >= std::numeric_limits<int8_t>::min()) {
				kind = ValueKind::INT8;
			} else if (val >= std::numeric_limits<int16_t>::min()) {
				kind = ValueKind::INT16;
			} else if (val >= std::numeric_limits<int32_t>::min()) {
				kind = ValueKind::INT32;
			} else if (val >= std::numeric_limits<int64_t>::min()) {
				kind = ValueKind::INT64;
			} else {
				kind = ValueKind::INT128;
			}
		} else {
			int base{10};
			auto enc = token.token;
			if (enc.starts_with("0x")) {
				base = 16;
				enc  = enc.substr(2);
			} else if (enc.starts_with("0o")) {
				base = 8;
				enc  = enc.substr(2);
			} else if (enc.starts_with("0b")) {
				base = 2;
				enc  = enc.substr(2);
			}
			uint64_t val;
			auto [ptr, err] = std::from_chars(enc.data(), enc.data() + enc.length(), val, base);
			if (err != std::errc{}) {
				throw file_error("Unable to parse integer literal", pos);
			}
			if (val <= std::numeric_limits<int8_t>::max()) {
				kind = ValueKind::INT8;
			} else if (val <= std::numeric_limits<uint8_t>::max()) {
				kind = ValueKind::UINT8;
			} else if (val <= std::numeric_limits<int16_t>::max()) {
				kind = ValueKind::INT16;
			} else if (val <= std::numeric_limits<uint16_t>::max()) {
				kind = ValueKind::UINT16;
			} else if (val <= std::numeric_limits<int32_t>::max()) {
				kind = ValueKind::INT32;
			} else if (val <= std::numeric_limits<uint32_t>::max()) {
				kind = ValueKind::UINT32;
			} else if (val <= std::numeric_limits<int64_t>::max()) {
				kind = ValueKind::INT64;
			} else if (val <= std::numeric_limits<uint64_t>::max()) {
				kind = ValueKind::UINT64;
			} else {
				kind = ValueKind::UINT128;
			}
		}
	} else if (token.type == TokenType::TLiteralDecimal) {
		// you could try to be smart about this as well, but i'll just check range errors in conversions
		// we can use this because the supported fromat is a subset of c++'s format

		char* dummy;
		auto value = token.token;

		{
			float fres{};
			auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), fres);
			if (ec == std::errc{}) {
				kind = ValueKind::FLOAT32;
				return;
			} else if (ec != std::errc::result_out_of_range) {
				throw file_error("Invalid numeric literal", token.at);
			}
		}

		{
			double dres{};
			auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), dres);
			if (ec == std::errc{}) {
				kind = ValueKind::FLOAT64;
				return;
			} else if (ec != std::errc::result_out_of_range) {
				throw file_error("Invalid numeric literal", token.at);
			}
		}

		{
			long double ldres{};
			auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), ldres);
			if (ec == std::errc{}) {
				kind = ValueKind::FLOAT128;
				return;
			} else if (ec != std::errc::result_out_of_range) {
				throw file_error("Invalid numeric literal", token.at);
			} else {
				throw file_error("Numeric literal out of range", token.at);
			}
		}
	} else if (token.type == TokenType::TIdentifier) {
		kind = ValueKind::LABEL;
	} else {
		throw file_error("Invalid token type in Value::Value", token.at);
	}
}

FunctionCall::FunctionCall(Token const& token)
: Value(token)
{
	kind = ValueKind::LABEL;
}

FunctionCall::FunctionCall(Value const& label)
: Value(label)
{
	kind = ValueKind::LABEL;
}

DiscreteType::DiscreteType(Token const& token)
{
	this->declpos = token.at;
	this->labels  = {token.token};
	if (token.token == *Keywords::BOOL) {
		this->size = 1;
		this->tid  = "b";
	} else if (token.token == *Keywords::U8) {
		this->size = 1;
		this->tid  = "i1";
	} else if (token.token == *Keywords::U16) {
		this->size = 2;
		this->tid  = "i2";
	} else if (token.token == *Keywords::U32) {
		this->size = 4;
		this->tid  = "i4";
	} else if (token.token == *Keywords::U64) {
		this->size = 8;
		this->tid  = "i8";
	} else if (token.token == *Keywords::U128 || token.token == *Keywords::UINT) {
		this->size = 16;
		this->tid  = "iF";
	} else if (token.token == *Keywords::S8) {
		this->size = 1;
		this->tid  = "I1";
	} else if (token.token == *Keywords::S16) {
		this->size = 2;
		this->tid  = "I2";
	} else if (token.token == *Keywords::S32) {
		this->size = 4;
		this->tid  = "I4";
	} else if (token.token == *Keywords::S64) {
		this->size = 8;
		this->tid  = "I8";
	} else if (token.token == *Keywords::S128 || token.token == *Keywords::SINT) {
		this->size = 16;
		this->tid  = "IF";
	} else if (token.token == *Keywords::F32) {
		this->size = 4;
		this->tid  = "f4";
	} else if (token.token == *Keywords::F64) {
		this->size = 8;
		this->tid  = "f8";
	} else if (token.token == *Keywords::F128 || token.token == *Keywords::FLOAT) {
		this->size = 16;
		this->tid  = "FF";
	} else if (token.token == *Keywords::C32) {
		this->size = 8;
		this->tid  = "F4";
	} else if (token.token == *Keywords::C64) {
		this->size = 16;
		this->tid  = "F8";
	} else if (token.token == *Keywords::C128 || token.token == *Keywords::COMPLEX) {
		this->size = 32;
		this->tid  = "FF";
	} else if (token.token == *Keywords::ADDRESS) {
		this->size = sizeof(void*);
		this->tid  = "p";
	} else if (token.token == *Keywords::HANDLE) {
		this->size = sizeof(void*);
		this->tid  = "P";
	} else if (token.token == *Keywords::STR) {
		this->tid = "s";
	} else if (token.token == *Keywords::AUTO) {
		this->tid = "^";
	} else if (token.token == *Keywords::ANY) {
		this->tid = "a";
	}
}
