/**
* Copyright (c) 2008 Moritz Bechler
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
**/


#include "php.h"
#include "php_krb5.h"
#include "compat.h"


/* {{{ */
void php_krb5_gssapi_handle_error(OM_uint32 major, OM_uint32 minor TSRMLS_DC)
{
	OM_uint32 error_context = 0;
	OM_uint32 minor_status = 0;
	gss_buffer_desc error_buffer;

	gss_display_status (&minor_status, major, GSS_C_GSS_CODE,
					GSS_C_NO_OID, &error_context, &error_buffer);


	while(error_context) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s (%ld,%ld)", (char*) error_buffer.value, (unsigned long int) major, (unsigned long int)minor);
		gss_release_buffer(&minor_status, &error_buffer);
		gss_display_status (&minor_status, major, GSS_C_GSS_CODE,
				GSS_C_NO_OID, &error_context, &error_buffer);
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s (%ld,%ld)", (char*) error_buffer.value,  (unsigned long int) major,  (unsigned long int)minor);
	gss_release_buffer(&minor_status, &error_buffer);

	if(minor) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "GSSAPI mechanism error #%ld",  (unsigned long int) minor);
		gss_display_status (&minor_status, minor, GSS_C_MECH_CODE,
					GSS_C_NO_OID, &error_context, &error_buffer);

		while(error_context) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", (char*) error_buffer.value);
			gss_release_buffer(&minor_status, &error_buffer);

			gss_display_status (&minor_status, minor_status, GSS_C_MECH_CODE,
                                                GSS_C_NO_OID, &error_context, &error_buffer);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s (%ld)", (char*)  error_buffer.value,  (unsigned long int) minor);
		gss_release_buffer(&minor_status, &error_buffer);
	}
}
/* }}} */
