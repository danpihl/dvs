#ifndef MAIN_APPLICATION_MISC_MISC_H_
#define MAIN_APPLICATION_MISC_MISC_H_

#include <string>

int getIntExponent(const double d);
double getBase(const double num);

std::string stripNumberFromLastZeros(const std::string& input_num);
std::string toStringWithNumDecimalPlaces(const double input_val, const size_t n = 6);
std::string formatNumber(const double num, const size_t n = 6);
std::string formatNumberInternal(const double num, const size_t n);
std::string zeroLStrip(const std::string& input_string);
std::string zeroRStrip(const std::string& input_string);

#endif  // MAIN_APPLICATION_MISC_MISC_H_
