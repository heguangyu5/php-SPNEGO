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

#include "php_krb5.h"
#include "php_krb5_gssapi.h"
#include "compat.h"
#include "SAPI.h"
#include "ext/standard/base64.h"
#include <math.h>
#include <netdb.h>
#include <sys/socket.h>

/* Class definition */
zend_object_handlers krb5_negotiate_auth_handlers;

zend_class_entry *krb5_ce_negotiate_auth;
typedef struct _krb5_negotiate_auth_object {
#if PHP_MAJOR_VERSION < 7
	zend_object std;
#endif
	gss_name_t servname;
	gss_name_t authed_user;
	gss_cred_id_t delegated;
#if PHP_MAJOR_VERSION >= 7
	zend_object std;
#endif
} krb5_negotiate_auth_object;


#if PHP_MAJOR_VERSION < 7
static void php_krb5_negotiate_auth_object_dtor(void *obj, zend_object_handle handle TSRMLS_DC);
zend_object_value php_krb5_negotiate_auth_object_new(zend_class_entry *ce TSRMLS_DC);
#else
static void php_krb5_negotiate_auth_object_free(zend_object *obj TSRMLS_DC);
zend_object *php_krb5_ticket_object_new(zend_class_entry *ce TSRMLS_DC);
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5NegotiateAuth_none, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5NegotiateAuth__construct, 0, 0, 1)
	ZEND_ARG_INFO(0, keytab)
ZEND_END_ARG_INFO()



PHP_METHOD(KRB5NegotiateAuth, __construct);
PHP_METHOD(KRB5NegotiateAuth, doAuthentication);
PHP_METHOD(KRB5NegotiateAuth, getAuthenticatedUser);

static zend_function_entry krb5_negotiate_auth_functions[] = {
	PHP_ME(KRB5NegotiateAuth, __construct,             arginfo_KRB5NegotiateAuth__construct,              ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(KRB5NegotiateAuth, doAuthentication,        arginfo_KRB5NegotiateAuth_none,                    ZEND_ACC_PUBLIC)
	PHP_ME(KRB5NegotiateAuth, getAuthenticatedUser,    arginfo_KRB5NegotiateAuth_none,                    ZEND_ACC_PUBLIC)
	PHP_FE_END
};


/** Registration **/
/* {{{ */
#if PHP_MAJOR_VERSION < 7
static void php_krb5_negotiate_auth_object_dtor(void *obj, zend_object_handle handle TSRMLS_DC)
{
	krb5_negotiate_auth_object *object = (krb5_negotiate_auth_object*)obj;

	OBJECT_STD_DTOR(object->std);

	if ( object->servname ) {
		free(object->servname);
	}
	efree(object);
}
#else
static void php_krb5_negotiate_auth_object_free(zend_object *obj TSRMLS_DC)
{
	krb5_negotiate_auth_object *object = (krb5_negotiate_auth_object*)((char *)obj - XtOffsetOf(krb5_negotiate_auth_object, std));

	if ( object->servname ) {
		free(object->servname);
	}
	zend_object_std_dtor(obj);
}
#endif
/* }}} */


static void setup_negotiate_auth(krb5_negotiate_auth_object *object TSRMLS_DC) {
	object->authed_user = GSS_C_NO_NAME;
	object->servname = GSS_C_NO_NAME;
	object->delegated = GSS_C_NO_CREDENTIAL;
}

/* {{{ */
#if PHP_MAJOR_VERSION < 7
zend_object_value php_krb5_negotiate_auth_object_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	krb5_negotiate_auth_object *object;
	object = emalloc(sizeof(krb5_negotiate_auth_object));

	setup_negotiate_auth(object TSRMLS_CC);

	INIT_STD_OBJECT(object->std, ce);

#if PHP_VERSION_ID < 50399
	zend_hash_copy(object->std.properties, &ce->default_properties,
	        		(copy_ctor_func_t) zval_add_ref, NULL,
					sizeof(zval*));
#else
	object_properties_init(&(object->std), ce);
#endif

	retval.handle = zend_objects_store_put(object, php_krb5_negotiate_auth_object_dtor, NULL, NULL TSRMLS_CC);

	retval.handlers = &krb5_negotiate_auth_handlers;
	return retval;
}
#else
zend_object *php_krb5_negotiate_auth_object_new(zend_class_entry *ce TSRMLS_DC)
{
	krb5_negotiate_auth_object *object;
	object = ecalloc(1, sizeof(krb5_negotiate_auth_object) + zend_object_properties_size(ce));

	setup_negotiate_auth(object TSRMLS_CC);

	zend_object_std_init(&object->std, ce TSRMLS_CC);
	object_properties_init(&object->std, ce);
	object->std.handlers = &krb5_negotiate_auth_handlers;
	return &object->std;
}
#endif
/* }}} */

/* {{{ */
int php_krb5_negotiate_auth_register_classes(TSRMLS_D) {
	zend_class_entry negotiate_auth;

	INIT_CLASS_ENTRY(negotiate_auth, "KRB5NegotiateAuth", krb5_negotiate_auth_functions);
	krb5_ce_negotiate_auth = zend_register_internal_class(&negotiate_auth TSRMLS_CC);
	krb5_ce_negotiate_auth->create_object = php_krb5_negotiate_auth_object_new;
	memcpy(&krb5_negotiate_auth_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
#if PHP_MAJOR_VERSION >= 7
	krb5_negotiate_auth_handlers.offset = XtOffsetOf(krb5_negotiate_auth_object, std);
	krb5_negotiate_auth_handlers.free_obj = php_krb5_negotiate_auth_object_free;
#endif

	return SUCCESS;
}
/* }}} */


/** KRB5NegotiateAuth Methods **/
/* {{{ proto bool KRB5NegotiateAuth::__construct( string $keytab )
   Initialize KRB5NegotitateAuth object with a keytab to use  */
PHP_METHOD(KRB5NegotiateAuth, __construct)
{
	char *keytab;
	strsize_t keytab_len = 0;

	KRB5_SET_ERROR_HANDLING(EH_THROW);
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, ARG_PATH, &keytab, &keytab_len) == FAILURE) {
		RETURN_FALSE;
	}
	KRB5_SET_ERROR_HANDLING(EH_NORMAL);

	if(krb5_gss_register_acceptor_identity(keytab) != GSS_S_COMPLETE) {
		zend_throw_exception(NULL, "Failed to use credential cache", 0 TSRMLS_CC);
		return;
	}
} /* }}} */

/* {{{ proto bool KRB5NegotiateAuth::doAuthentication( string $authorization )
   Performs Negotiate/GSSAPI authentication  */
PHP_METHOD(KRB5NegotiateAuth, doAuthentication)
{
	zend_string *token = NULL;
	krb5_negotiate_auth_object *object;

	OM_uint32 status = 0;
	OM_uint32 minor_status = 0;
	OM_uint32 flags;
	gss_ctx_id_t gss_context = GSS_C_NO_CONTEXT;
	gss_buffer_desc input_token;
	gss_buffer_desc output_token;
	gss_cred_id_t server_creds = GSS_C_NO_CREDENTIAL;

    char *str;
    size_t str_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
        RETURN_FALSE;
    }
    if(strncasecmp(str, "negotiate", 9) != 0) {
        // user did not provide negotiate authentication data
        RETURN_FALSE;
    }
    if (str_len < 11) {
        // user gave negotiate header but no data
        RETURN_FALSE;
    }
    #if PHP_MAJOR_VERSION < 7
        int ret_len = 0;
        char *result = (char *)php_base64_decode_ex((unsigned char *)(str + 10), str_len - 10, &ret_len, 1);
        token = zend_string_init(result, ret_len, 0);
        efree(result);
    #else
        token = php_base64_decode_ex(str + 10, str_len - 10, 1);
    #endif

    if(!token) {
        RETURN_FALSE;
    }

	object = KRB5_THIS_NEGOTIATE_AUTH;
	if(!object) {
		RETURN_FALSE;
	}

	minor_status = 0;
	input_token.length = token->len;
	input_token.value = token->val;

	status = gss_accept_sec_context(   &minor_status,
                                       &gss_context,
                                       server_creds,
                                       &input_token,
                                       GSS_C_NO_CHANNEL_BINDINGS,
                                       &object->authed_user,
                                       NULL,
                                       &output_token,
                                       &flags,
                                       NULL,
                                       &object->delegated);


	if(!(flags & GSS_C_DELEG_FLAG)) {
		object->delegated = GSS_C_NO_CREDENTIAL;
	}

	zend_string_release(token);

	if(GSS_ERROR(status)) {
		php_krb5_gssapi_handle_error(status, minor_status TSRMLS_CC);
		zend_throw_exception(NULL, "Error while accepting security context", status TSRMLS_CC);
		RETURN_FALSE;
	}

	if(gss_context != GSS_C_NO_CONTEXT) {
		gss_delete_sec_context(&minor_status, &gss_context, GSS_C_NO_BUFFER);
	}

	if(output_token.length > 0) {

#if PHP_MAJOR_VERSION < 7
		ret_len = 0;
		result = (char *)php_base64_encode((unsigned char *)output_token.value, output_token.length, &ret_len);
		zend_string *encoded = zend_string_init(result, ret_len, 0);
		efree(result);
#else
		zend_string *encoded = php_base64_encode(output_token.value, output_token.length);
#endif

		sapi_header_line ctr = {0};
		ctr.line_len = strlen("WWW-Authenticate: ") + encoded->len;
		ctr.line = emalloc(ctr.line_len + 1);
		ctr.line[ctr.line_len] = 0;
		strcpy(ctr.line, "WWW-Authenticate: ");
		strncpy(ctr.line + strlen("WWW-Authenticate: "), encoded->val, encoded->len);
		ctr.response_code = 200;
		sapi_header_op(SAPI_HEADER_ADD, &ctr TSRMLS_CC);

		zend_string_release(encoded);
		efree(ctr.line);
		gss_release_buffer(&minor_status, &output_token);
	}
	RETURN_TRUE;
} /* }}} */

/* {{{ proto string KRB5NegotiateAuth::getAuthenticatedUser(  )
   Gets the principal name of the authenticated user  */
PHP_METHOD(KRB5NegotiateAuth, getAuthenticatedUser)
{
	OM_uint32 status, minor_status;
	krb5_negotiate_auth_object *object;

	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_FALSE;
	}
	object = KRB5_THIS_NEGOTIATE_AUTH;

	if(!object || !object->authed_user || object->authed_user == GSS_C_NO_NAME) {
		RETURN_FALSE;
	}

	gss_buffer_desc username_tmp;
	status = gss_display_name(&minor_status, object->authed_user, &username_tmp, NULL);

	if(GSS_ERROR(status)) {
		php_krb5_gssapi_handle_error(status, minor_status TSRMLS_CC);
		RETURN_FALSE;
	}

	_ZVAL_STRINGL(return_value, username_tmp.value, username_tmp.length);
	gss_release_buffer(&minor_status, &username_tmp);
} /* }}} */
