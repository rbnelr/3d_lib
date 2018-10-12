
#include "stdio.h"
#include "stdarg.h"

#include "mylibs/basic_typedefs.hpp"
#include "mylibs/string.hpp"

namespace engine {
	using namespace basic_typedefs;
	using namespace string;
	
	//
	class Console {
		
	};

	void errprint (cstr format, ...) {
		
		std::string str;
		{
			va_list vl;
			va_start(vl, format);

			vprints(&str, format, vl);

			va_end(vl);
		}



		fprintf(stderr, str.c_str());
	}
}
