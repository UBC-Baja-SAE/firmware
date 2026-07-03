# Workaround for an explicit-specialization ordering bug in foxglove-sdk's
# parameter.hpp. Two inline member templates, ParameterView::isArray<T>()
# and ParameterView::isDict<T>(), call this->get<ParameterValueView::Array>()
# / this->get<ParameterValueView::Dict>() with a fixed (non-dependent) type.
# Under strict two-phase lookup (MSVC /permissive-, which Qt requires; also
# GCC/Clang in conformant mode) that non-dependent call is resolved while the
# class body itself is being parsed -- before the explicit specializations of
# ParameterView::get<T>() later in the header are visible. This produces
# C2908/C3856 on MSVC ("explicit specialization ... has already been
# instantiated").
#
# Fix: declare isArray<T>()/isDict<T>() in-class as prototypes only, and
# define their bodies out-of-line, after all the get<T> specializations.
# This is a legal reordering under the standard -- it doesn't touch behavior,
# just moves where the two-phase lookup can see what it needs.

file(READ "${FILE}" CONTENTS)

set(GUARD_STRING "Patched: deferred out-of-line")
string(FIND "${CONTENTS}" "${GUARD_STRING}" ALREADY_PATCHED)
if(NOT ALREADY_PATCHED EQUAL -1)
    message(STATUS "foxglove parameter.hpp already patched, skipping")
    return()
endif()

# --- 1. Replace inline isArray<T>() body with a declaration ---
set(OLD_ISARRAY "  template<typename T>
  [[nodiscard]] bool isArray() const noexcept {
    if (!this->isArray<ParameterValueView>()) {
      return false;
    }
    try {
      const auto& arr = this->get<ParameterValueView::Array>();
      return std::all_of(arr.begin(), arr.end(), [](const ParameterValueView& elem) noexcept {
        return elem.is<T>();
      });
    } catch (...) {
      return false;
    }
  }")
set(NEW_ISARRAY "  template<typename T>
  [[nodiscard]] bool isArray() const noexcept;")

string(FIND "${CONTENTS}" "${OLD_ISARRAY}" ISARRAY_FOUND)
if(ISARRAY_FOUND EQUAL -1)
    message(FATAL_ERROR "foxglove parameter.hpp: isArray<T>() body not found - header may have changed upstream, update patch_foxglove_parameter.cmake")
endif()
string(REPLACE "${OLD_ISARRAY}" "${NEW_ISARRAY}" CONTENTS "${CONTENTS}")

# --- 2. Replace inline isDict<T>() body with a declaration ---
set(OLD_ISDICT "  template<typename T>
  [[nodiscard]] bool isDict() const noexcept {
    if (!this->isDict<ParameterValueView>()) {
      return false;
    }
    try {
      const auto& dict = this->get<ParameterValueView::Dict>();
      return std::all_of(
        dict.begin(),
        dict.end(),
        [](const std::pair<std::string, ParameterValueView>& elem) noexcept {
          return elem.second.is<T>();
        }
      );
    } catch (...) {
      return false;
    }
  }")
set(NEW_ISDICT "  template<typename T>
  [[nodiscard]] bool isDict() const noexcept;")

string(FIND "${CONTENTS}" "${OLD_ISDICT}" ISDICT_FOUND)
if(ISDICT_FOUND EQUAL -1)
    message(FATAL_ERROR "foxglove parameter.hpp: isDict<T>() body not found - header may have changed upstream, update patch_foxglove_parameter.cmake")
endif()
string(REPLACE "${OLD_ISDICT}" "${NEW_ISDICT}" CONTENTS "${CONTENTS}")

# --- 3. Insert the out-of-line definitions right after the get<T> dict specialization ---
set(INSERTION_POINT "template<>
[[nodiscard]] inline std::map<std::string, ParameterValueView> ParameterView::get() const {
  return this->getDict<ParameterValueView>();
}
")

string(FIND "${CONTENTS}" "${INSERTION_POINT}" INSERT_FOUND)
if(INSERT_FOUND EQUAL -1)
    message(FATAL_ERROR "foxglove parameter.hpp: get<T> dict specialization not found - header may have changed upstream, update patch_foxglove_parameter.cmake")
endif()

set(OUT_OF_LINE "
// --- Patched: deferred out-of-line so non-dependent get<Array>/get<Dict>
// calls resolve after the specializations above are visible ---
template<typename T>
[[nodiscard]] bool ParameterView::isArray() const noexcept {
  if (!this->isArray<ParameterValueView>()) {
    return false;
  }
  try {
    const auto& arr = this->get<ParameterValueView::Array>();
    return std::all_of(arr.begin(), arr.end(), [](const ParameterValueView& elem) noexcept {
      return elem.is<T>();
    });
  } catch (...) {
    return false;
  }
}

template<typename T>
[[nodiscard]] bool ParameterView::isDict() const noexcept {
  if (!this->isDict<ParameterValueView>()) {
    return false;
  }
  try {
    const auto& dict = this->get<ParameterValueView::Dict>();
    return std::all_of(
      dict.begin(),
      dict.end(),
      [](const std::pair<std::string, ParameterValueView>& elem) noexcept {
        return elem.second.is<T>();
      }
    );
  } catch (...) {
    return false;
  }
}
")

string(REPLACE "${INSERTION_POINT}" "${INSERTION_POINT}${OUT_OF_LINE}" CONTENTS "${CONTENTS}")

file(WRITE "${FILE}" "${CONTENTS}")
message(STATUS "Patched foxglove parameter.hpp (deferred isArray<T>/isDict<T> out-of-line)")