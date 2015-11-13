/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file error.h
/// Exceptions for SCRAM.

#ifndef SCRAM_SRC_ERROR_H_
#define SCRAM_SRC_ERROR_H_

#include <exception>
#include <string>

namespace scram {

/// @class Error
/// The Error class is the base class
/// for common exceptions specific to the SCRAM code.
class Error : public std::exception {
 public:
  /// Constructs a new error with a provided message.
  ///
  /// @param[in] msg  The message to be passed with this error.
  explicit Error(std::string msg);

  Error(const Error&) = default;  ///< Explicit declaration.

  virtual ~Error() noexcept = default;

  /// @returns The error message.
  const char* what() const noexcept override;

  /// @returns The error message.
  const std::string& msg() const { return msg_; }

  /// Sets the error message.
  ///
  /// @param[in] msg  The error message.
  void msg(std::string msg) {
    msg_ = msg;
    thrown_ = kPrefix_ + msg;
  }

 protected:
  /// The error message.
  std::string msg_;

 private:
  static const std::string kPrefix_;  ///< Prefix specific to SCRAM.
  std::string thrown_;  ///< The message to throw with the prefix.
};

/// @class ValueError
/// For values that are not acceptable.
/// For example, negative probability.
class ValueError : public Error {
 public:
  using Error::Error;  ///< An error with a message.
};

/// @class ValidationError
/// For validating input parameters or user arguments.
class ValidationError : public Error {
 public:
  using Error::Error;  ///< An error with a message.
};

/// @class RedefinitionError
/// For cases when events or practically anything is redefined.
class RedefinitionError : public ValidationError {
 public:
  using ValidationError::ValidationError;  ///< An error with a message.
};

/// @class DuplicateArgumentError
/// This error indicates that arguments must be unique.
class DuplicateArgumentError : public ValidationError {
 public:
  using ValidationError::ValidationError;  ///< An error with a message.
};

/// @class IOError
/// For input/output related errors.
class IOError : public Error {
 public:
  using Error::Error;  ///< An error with a message.
};

/// @class InvalidArgument
/// This error class can be used
/// to indicate unacceptable arguments.
class InvalidArgument : public Error {
 public:
  using Error::Error;  ///< An error with a message.
};

/// @class LogicError
/// Signals internal logic errors,
/// for example, pre-condition failure
/// or use of functionality in ways not designed to.
class LogicError : public Error {
 public:
  using Error::Error;  ///< An error with a message.
};

/// @class IllegalOperation
/// This error can be used to indicate
/// that call for a function or operation is not legal.
/// For example, a derived class can make illegal
/// the call of the virtual function of the base class.
class IllegalOperation : public Error {
 public:
  using Error::Error;  ///< An error with a message.
};

}  // namespace scram

#endif  // SCRAM_SRC_ERROR_H_
