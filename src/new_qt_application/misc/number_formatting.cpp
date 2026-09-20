#include <cmath>
#include <iostream>
#include <sstream>

#include "misc/misc.h"

int getIntExponent(const double d)  // TODO: Change to std::int32_t
{
    if (d == 0.0)
    {
        return 0;
    }
    else
    {
        return std::floor(std::log10(std::fabs(d)));
    }
}

double getBase(const double num)
{
    const double exp_val = getIntExponent(num);

    return num / std::pow(10.0, exp_val);
}

/*std::string getBaseAsString(const double num)
{
    const double exp_val = getIntExponent(num);

    return toStringWithNumDecimalPlaces(num / std::pow(10.0, exp_val), 5);
}

std::string getIntExponentAsString(const double num)
{
    return std::to_string(getIntExponent(num));
}*/

std::string toStringWithNumDecimalPlaces(const double input_val, const size_t n)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << input_val;
    return out.str();
}

std::string formatNumberInternal(const double num, const size_t n)
{
    const double abs_num = std::abs(num);
    if (abs_num == 0.0)
    {
        return "0.0";
    }

    const int exp_num = getIntExponent(num);

    // if (abs_num > 1.0e4)
    if (exp_num > 4)
    {
        return toStringWithNumDecimalPlaces(getBase(num), n) + "e" + "+" + std::to_string(exp_num);
    }
    // else if (abs_num < 1.0e-2)
    else if (exp_num < -2)
    {
        return toStringWithNumDecimalPlaces(getBase(num), n) + "e" + std::to_string(exp_num);
    }
    else
    {
        return toStringWithNumDecimalPlaces(num, 5 - exp_num);
    }
}

std::string zeroLStrip(const std::string& input_string)
{
    if (input_string.empty())
    {
        return "";
    }

    const size_t string_length = input_string.length();

    auto start_it = input_string.begin();
    auto end_it = input_string.rbegin();

    size_t idx = 0U;
    while ((*start_it == '0') && (idx < (string_length - 1U)))
    {
        ++start_it;
        idx++;
    }

    return std::string(start_it, end_it.base());
}

std::string zeroRStrip(const std::string& input_string)
{
    if (input_string.empty())
    {
        return "";
    }

    const size_t string_length = input_string.length();

    auto start_it = input_string.begin();
    auto end_it = input_string.rbegin();

    size_t idx = string_length - 1U;

    while ((*end_it == '0') && (idx != 0U) && (idx < string_length))
    {
        ++end_it;
        idx--;
    }
    return std::string(start_it, end_it.base());
}

std::string stripNumberFromLastZeros(const std::string& input_num)
{
    if (input_num.length() == 0U)
    {
        throw std::runtime_error("Input string is empty.");
    }
    else if (input_num == ".")
    {
        throw std::runtime_error("Input string is a single dot.");
    }
    else if (input_num == "0")
    {
        return "0.0";
    }

    const std::string input_num_sanitized = input_num[0] == '-' ? input_num.substr(1) : input_num;

    const size_t e_pos = input_num_sanitized.find('e');
    const size_t comma_pos = input_num_sanitized.find('.');

    std::string before_comma{""};
    std::string after_comma{""};
    std::string exponent{""};
    std::string sign{""};

    const bool has_comma = comma_pos != std::string::npos;
    const bool has_e = e_pos != std::string::npos;

    if (has_comma && has_e)
    {
        before_comma = input_num_sanitized.substr(0, comma_pos);
        after_comma = input_num_sanitized.substr(comma_pos + 1, e_pos - comma_pos - 1);
        exponent = input_num_sanitized.substr(e_pos);

        if (after_comma.length() == 0U)
        {
            after_comma = "0";
        }
    }
    else if (has_comma && !has_e)
    {
        before_comma = input_num_sanitized.substr(0, comma_pos);
        after_comma = input_num_sanitized.substr(comma_pos + 1);
        if (after_comma.length() == 0U)
        {
            after_comma = "0";
        }
    }
    else if (has_e && !has_comma)
    {
        before_comma = input_num_sanitized.substr(0, e_pos);
        exponent = input_num_sanitized.substr(e_pos);
    }
    else
    {
        before_comma = input_num_sanitized;
    }

    if (before_comma.length() == 0U)
    {
        before_comma = "0";
    }

    if (has_e)
    {
        // Remove the e
        exponent = exponent.substr(1);
        if (exponent.length() == 0U)
        {
            throw std::runtime_error("Exponent is empty.");
        }

        if (exponent[0] == '+')
        {
            sign = "+";
            exponent = exponent.substr(1);
        }
        else if (exponent[0] == '-')
        {
            sign = "-";
            exponent = exponent.substr(1);
        }
        else
        {
            sign = "+";
        }
    }

    before_comma = zeroLStrip(before_comma);
    after_comma = zeroRStrip(after_comma);
    exponent = zeroLStrip(exponent);

    if (has_e)
    {
        exponent = "e" + sign + exponent;
        if (exponent == "e+0" || exponent == "e-0" || exponent == "e0")
        {
            exponent = "";
        }
    }

    if (has_comma)
    {
        after_comma = "." + after_comma;
    }
    else
    {
        after_comma = ".0";
    }

    if (input_num[0] == '-')
    {
        before_comma = "-" + before_comma;
    }

    const std::string res = before_comma + after_comma + exponent;

    return res;
}

std::string formatNumber(const double num, const size_t n)
{
    const std::string s = formatNumberInternal(num, n);

    return stripNumberFromLastZeros(s);
}
