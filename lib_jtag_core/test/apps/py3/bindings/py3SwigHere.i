// py3SwigHere.i
%module pySwigHere

%{
#define SWIG_FILE_WITH_INIT
%}

%init %{
%}

%include "std_vector.i"
%include "std_string.i"

%{
#include "py3SwigHere.h"
%}

%template(c2json) std::vector<std::string>;

%extend std::vector<std::string> {
  void empty_and_delete() {
    for (std::vector<std::string>::iterator it = $self->begin(); 
         it != $self->end(); ++it) {
      // if it was a pointer, we could delete *it;
    }
    $self->clear();
  }
}

%include "py3SwigHere.h"
