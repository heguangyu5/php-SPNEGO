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

/* CHANGELOG (more recent on top)
 *
 * 2010-05-27 Mark Seecof	add getPrincipal(), getRealm(), getLifetime(),
 * 				renew(), getTktAttrs() methods; support more
 * 				options to initKeytab/Password(); check only
 * 				primary TGT in isValid(); fix some bugs.
 *
 * 2010-04-11 Moritz Bechler	RC2 release
 */

#include "config.h"
#include "php_krb5.h"
#include "compat.h"

#include "ext/standard/info.h"
#include "ext/standard/base64.h"

#include <sys/time.h>
#include <arpa/inet.h>

#ifdef HAVE_KADM5
#include "kdb.h"
#endif


/* Class definition */
zend_module_entry krb5_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_KRB5_EXT_NAME,
    NULL,
    PHP_MINIT(krb5),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(krb5),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_KRB5_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_KRB5
	ZEND_GET_MODULE(krb5)
#endif


krb5_error_code php_krb5_display_error(krb5_context ctx, krb5_error_code code, char* str TSRMLS_DC);

PHP_MINIT_FUNCTION(krb5)
{
	/* register constants */
	REGISTER_LONG_CONSTANT("GSS_C_DELEG_FLAG", GSS_C_DELEG_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_MUTUAL_FLAG", GSS_C_MUTUAL_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_REPLAY_FLAG", GSS_C_REPLAY_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_SEQUENCE_FLAG", GSS_C_SEQUENCE_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_CONF_FLAG", GSS_C_CONF_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_INTEG_FLAG", GSS_C_INTEG_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_ANON_FLAG", GSS_C_ANON_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_PROT_READY_FLAG", GSS_C_PROT_READY_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_TRANS_FLAG", GSS_C_TRANS_FLAG, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT("GSS_C_BOTH", GSS_C_BOTH, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_INITIATE", GSS_C_INITIATE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_ACCEPT", GSS_C_ACCEPT, CONST_CS | CONST_PERSISTENT );

#ifdef KRB5_TL_DB_ARGS
	REGISTER_LONG_CONSTANT("KRB5_TL_DB_ARGS", KRB5_TL_DB_ARGS, CONST_CS | CONST_PERSISTENT );
#endif

	if(php_krb5_negotiate_auth_register_classes(TSRMLS_C) != SUCCESS) {
		return FAILURE;
	}

	return SUCCESS;
}


PHP_MINFO_FUNCTION(krb5)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Kerberos 5 support", "enabled");
	php_info_print_table_row(2, "Extension version", PHP_KRB5_VERSION);
#ifdef HAVE_KRB5_MIT
	php_info_print_table_row(2, "Kerberos library", "MIT");
#endif

#ifdef HAVE_KRB5_HEIMDAL
	php_info_print_table_row(2, "Kerberos library", "Heimdal");
#endif

#ifdef KRB5_VERSION
	php_info_print_table_row(2, "Library version", KRB5_VERSION);
#endif

#ifdef HAVE_KADM5
	php_info_print_table_row(2, "KADM5 support", "yes");
#else
	php_info_print_table_row(2, "KADM5 support", "no");
#endif

	php_info_print_table_row(2, "GSSAPI/SPNEGO auth support", "yes");
	php_info_print_table_end();
}

/* {{{ */
krb5_error_code php_krb5_display_error(krb5_context ctx, krb5_error_code code, char* str TSRMLS_DC) {
	const char *errstr = krb5_get_error_message(ctx,code);
	zend_throw_exception_ex(NULL, 0 TSRMLS_CC, str, errstr);
	krb5_free_error_message(ctx, errstr);
	return code;
}
/* }}} */

/* bottom of file */
