/**
 * The Epeg PHP extension
 *
 * Copyright (c) 2006-2010 Ryusuke SEKIYAMA. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @package     php-epeg
 * @author      Ryusuke SEKIYAMA <rsky0711@gmail.com>
 * @copyright   2006-2010 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 */

#include "php_epeg.h"

/* {{{ globals */

static int le_epeg;
static zend_class_entry *ce_Epeg = NULL;
static zend_object_handlers _php_epeg_object_handlers;

/* }}} */

/* {{{ module function prototypes */

static PHP_MINIT_FUNCTION(epeg);
static PHP_MINFO_FUNCTION(epeg);

/* }}} */

/* {{{ PHP function prototypes */

static PHP_FUNCTION(epeg_thumbnail_create);
static PHP_FUNCTION(epeg_open);
static PHP_FUNCTION(epeg_file_open);
static PHP_FUNCTION(epeg_memory_open);
static PHP_FUNCTION(epeg_size_get);
static PHP_FUNCTION(epeg_decode_size_set);
#ifdef PHP_EPEG_ENABLE_DECODE_BOUNDS_SET
static PHP_FUNCTION(epeg_decode_bounds_set);
#endif
static PHP_FUNCTION(epeg_decode_colorspace_set);
static PHP_FUNCTION(epeg_comment_get);
static PHP_FUNCTION(epeg_comment_set);
static PHP_FUNCTION(epeg_quality_set);
static PHP_FUNCTION(epeg_thumbnail_comments_get);
static PHP_FUNCTION(epeg_thumbnail_comments_enable);
static PHP_FUNCTION(epeg_encode);
static PHP_FUNCTION(epeg_trim);
static PHP_FUNCTION(epeg_close);

static PHP_METHOD(Epeg, openFile);
static PHP_METHOD(Epeg, openBuffer);

/* }}} */

/* {{{ internal function prototypes */

static inline int
round_to_i(double num)
{
	return (int)lround(num);
}

static php_epeg_t *
php_epeg_file_open(char *file TSRMLS_DC);

static php_epeg_t *
php_epeg_memory_open(char *data, int data_len TSRMLS_DC);

static void
php_epeg_open_wrapper(INTERNAL_FUNCTION_PARAMETERS, int mode);

static void
php_epeg_set_retval(unsigned char *buf, int buf_len,
		char *file, int file_len, zval *retval TSRMLS_DC);

static void
php_epeg_encode_error(int errcode TSRMLS_DC);

static void
php_epeg_trim_error(int errcode TSRMLS_DC);

static void
php_epeg_free_resource(zend_rsrc_list_entry *rsrc TSRMLS_DC);

static void
php_epeg_free(php_epeg_t *im);

static zend_object_value
php_epeg_object_new(zend_class_entry *ce TSRMLS_DC);

static void
php_epeg_free_object_storage(void *object TSRMLS_DC);

static int
php_epeg_calc_thumb_size(
		int src_width, int src_height,
		int max_width, int max_height,
		int *dst_width, int *dst_height);

static void
php_epeg_reset(php_epeg_t *im);

/* }}} */

/* {{{ function shortcurs */

/* {{{ macro for parsing parameters and fetching resource */

/* fetch (php_epeg_t *) from resource */
#define FETCH_IMAGE_FROM_RESOURCE(im, zim) \
	ZEND_FETCH_RESOURCE((im), php_epeg_t *, &(zim), -1, "epeg", le_epeg)

/* free any type of resource */
#define FREE_RESOURCE(resource) zend_list_delete(Z_RESVAL_P(resource))

/* fetch (php_epeg_t *) from object storage */
#define FETCH_IMAGE_FROM_OBJECT(im, obj) { \
	php_epeg_object *intern; \
	intern = (php_epeg_object *)zend_object_store_get_object((obj) TSRMLS_CC); \
	im = intern->ptr; \
}

/* expect a parameter */
#define PHP_EPEG_PARSE_PARAMETER() \
	if (obj) { \
		if (ZEND_NUM_ARGS() != 0) { \
			WRONG_PARAM_COUNT; \
		} \
		FETCH_IMAGE_FROM_OBJECT(im, obj); \
	} else { \
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zim) == FAILURE) { \
			return; \
		} \
		FETCH_IMAGE_FROM_RESOURCE(im, zim); \
	}

/* expect many parameters */
#define PHP_EPEG_PARSE_PARAMETERS(fmt, ...) \
	if (obj) { \
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, fmt, __VA_ARGS__) == FAILURE) { \
			return; \
		} \
		FETCH_IMAGE_FROM_OBJECT(im, obj); \
	} else { \
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r" fmt, &zim, __VA_ARGS__) == FAILURE) { \
			return; \
		} \
		FETCH_IMAGE_FROM_RESOURCE(im, zim); \
	}

/* }}} */

/* {{{ argument informations */

#if PHP_VERSION_ID < 50300
#define ARG_INFO_STATIC static
#else
#define ARG_INFO_STATIC
#endif

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg__epeg, 0)
	ZEND_ARG_INFO(0, image)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg__output, 0)
	ZEND_ARG_INFO(0, image)
	ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg__output_m, 0)
	ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_epeg_thumbnail_create, 0, 0, 4)
	ZEND_ARG_INFO(0, in_file)
	ZEND_ARG_INFO(0, out_file)
	ZEND_ARG_INFO(0, max_width)
	ZEND_ARG_INFO(0, max_height)
	ZEND_ARG_INFO(0, quality)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_epeg_open, 0, 0, 1)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, is_data)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_file_open, 0)
	ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_memory_open, 0)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_decode_size_set, 0)
	ZEND_ARG_INFO(0, image)
	ZEND_ARG_INFO(0, width)
	ZEND_ARG_INFO(0, height)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_decode_size_set_m, 0)
	ZEND_ARG_INFO(0, width)
	ZEND_ARG_INFO(0, height)
ZEND_END_ARG_INFO()

#ifdef PHP_EPEG_ENABLE_DECODE_BOUNDS_SET
ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_decode_bounds_set, 0)
	ZEND_ARG_INFO(0, image)
	ZEND_ARG_INFO(0, x)
	ZEND_ARG_INFO(0, y)
	ZEND_ARG_INFO(0, width)
	ZEND_ARG_INFO(0, height)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_decode_bounds_set_m, 0)
	ZEND_ARG_INFO(0, x)
	ZEND_ARG_INFO(0, y)
	ZEND_ARG_INFO(0, width)
	ZEND_ARG_INFO(0, height)
ZEND_END_ARG_INFO()
#endif

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_decode_colorspace_set, 0)
	ZEND_ARG_INFO(0, image)
	ZEND_ARG_INFO(0, colorspace)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_decode_colorspace_set_m, 0)
	ZEND_ARG_INFO(0, colorspace)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_comment_set, 0)
	ZEND_ARG_INFO(0, image)
	ZEND_ARG_INFO(0, comment)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_comment_set_m, 0)
	ZEND_ARG_INFO(0, comment)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_quality_set, 0)
	ZEND_ARG_INFO(0, image)
	ZEND_ARG_INFO(0, quality)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO(arginfo_epeg_quality_set_m, 0)
	ZEND_ARG_INFO(0, quality)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_epeg_thumbnail_comments_enable, 0, 0, 1)
	ZEND_ARG_INFO(0, image)
	ZEND_ARG_INFO(0, onoff)
ZEND_END_ARG_INFO()

ARG_INFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_epeg_thumbnail_comments_enable_m, 0, 0, 0)
	ZEND_ARG_INFO(0, onoff)
ZEND_END_ARG_INFO()

/* }}} */

/* {{{ Class definitions */

/* {{{ methods */

static zend_function_entry epeg_methods[] = {
	PHP_ME(Epeg, openFile,   arginfo_epeg_file_open,   ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(Epeg, openBuffer, arginfo_epeg_memory_open, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME_MAPPING(__construct,             epeg_open,                      arginfo_epeg_open,          ZEND_ACC_CTOR | ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(getSize,                 epeg_size_get,                  NULL,                                       ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(setDecodeSize,           epeg_decode_size_set,           arginfo_epeg_decode_size_set_m,             ZEND_ACC_PUBLIC)
#ifdef PHP_EPEG_ENABLE_DECODE_BOUNDS_SET
	PHP_ME_MAPPING(setDecodeBounds,         epeg_decode_bounds_set,         arginfo_epeg_decode_bounds_set_m,           ZEND_ACC_PUBLIC)
#endif
	PHP_ME_MAPPING(setDecodeColorSpace,     epeg_decode_colorspace_set,     arginfo_epeg_decode_colorspace_set_m,       ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(getComment,              epeg_comment_get,               NULL,                                       ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(setComment,              epeg_comment_set,               arginfo_epeg_comment_set_m,                 ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(setQuality,              epeg_quality_set,               arginfo_epeg_quality_set_m,                 ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(getThumbnailComments,    epeg_thumbnail_comments_get,    NULL,                                       ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(enableThumbnailComments, epeg_thumbnail_comments_enable, arginfo_epeg_thumbnail_comments_enable_m,   ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(encode,                  epeg_encode,                    arginfo_epeg__output_m,                     ZEND_ACC_PUBLIC)
	PHP_ME_MAPPING(trim,                    epeg_trim,                      arginfo_epeg__output_m,                     ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};
/* }}} */
/* }}} Class definitions */

/* {{{ epeg_functions[] */
static zend_function_entry epeg_functions[] = {
	PHP_FE(epeg_thumbnail_create,           arginfo_epeg_thumbnail_create)
	PHP_FE(epeg_open,                       arginfo_epeg_open)
	PHP_FE(epeg_file_open,                  arginfo_epeg_file_open)
	PHP_FE(epeg_memory_open,                arginfo_epeg_memory_open)
	PHP_FE(epeg_size_get,                   arginfo_epeg__epeg)
	PHP_FE(epeg_decode_size_set,            arginfo_epeg_decode_size_set)
#ifdef PHP_EPEG_ENABLE_DECODE_BOUNDS_SET
	PHP_FE(epeg_decode_bounds_set,          arginfo_epeg_decode_bounds_set)
#endif
	PHP_FE(epeg_decode_colorspace_set,      arginfo_epeg_decode_colorspace_set)
	PHP_FE(epeg_comment_get,                arginfo_epeg__epeg)
	PHP_FE(epeg_comment_set,                arginfo_epeg_comment_set)
	PHP_FE(epeg_quality_set,                arginfo_epeg_quality_set)
	PHP_FE(epeg_thumbnail_comments_get,     arginfo_epeg__epeg)
	PHP_FE(epeg_thumbnail_comments_enable,  arginfo_epeg_thumbnail_comments_enable)
	PHP_FE(epeg_encode,                     arginfo_epeg__output)
	PHP_FE(epeg_trim,                       arginfo_epeg__output)
	PHP_FE(epeg_close,                      arginfo_epeg__epeg)
	{ NULL, NULL, NULL }
};
/* }}} */

/* {{{ epeg_module_entry */
zend_module_entry epeg_module_entry = {
	STANDARD_MODULE_HEADER,
	"epeg",
	epeg_functions,
	PHP_MINIT(epeg),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(epeg),
	PHP_EPEG_MODULE_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_EPEG
ZEND_GET_MODULE(epeg)
#endif

#define PHP_EPEG_REGISTER_CONSTANT(name) \
	REGISTER_LONG_CONSTANT(#name, (long)name, CONST_PERSISTENT | CONST_CS)

#define PHP_EPEG_REGISTER_CLASS_CONSTANT(name) \
		zend_declare_class_constant_long(ce_Epeg, #name, strlen(#name), (long)EPEG_##name TSRMLS_CC)

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(epeg)
{
	zend_class_entry ce;

	PHP_EPEG_REGISTER_CONSTANT(EPEG_GRAY8);
	PHP_EPEG_REGISTER_CONSTANT(EPEG_YUV8);
	PHP_EPEG_REGISTER_CONSTANT(EPEG_RGB8);
	PHP_EPEG_REGISTER_CONSTANT(EPEG_BGR8);
	PHP_EPEG_REGISTER_CONSTANT(EPEG_RGBA8);
	PHP_EPEG_REGISTER_CONSTANT(EPEG_BGRA8);
	PHP_EPEG_REGISTER_CONSTANT(EPEG_ARGB32);
	PHP_EPEG_REGISTER_CONSTANT(EPEG_CMYK);

	le_epeg = zend_register_list_destructors_ex(php_epeg_free_resource, NULL, "epeg", module_number);

	INIT_CLASS_ENTRY(ce, "Epeg", epeg_methods);
	ce_Epeg = zend_register_internal_class(&ce TSRMLS_CC);
	if (!ce_Epeg) {
		return FAILURE;
	}
	ce_Epeg->create_object = php_epeg_object_new;

	memcpy(&_php_epeg_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	_php_epeg_object_handlers.clone_obj = NULL;

	PHP_EPEG_REGISTER_CLASS_CONSTANT(GRAY8);
	PHP_EPEG_REGISTER_CLASS_CONSTANT(YUV8);
	PHP_EPEG_REGISTER_CLASS_CONSTANT(RGB8);
	PHP_EPEG_REGISTER_CLASS_CONSTANT(BGR8);
	PHP_EPEG_REGISTER_CLASS_CONSTANT(RGBA8);
	PHP_EPEG_REGISTER_CLASS_CONSTANT(BGRA8);
	PHP_EPEG_REGISTER_CLASS_CONSTANT(ARGB32);
	PHP_EPEG_REGISTER_CLASS_CONSTANT(CMYK);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(epeg)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Epeg Support", "enabled");
	php_info_print_table_row(2, "Module Version", PHP_EPEG_MODULE_VERSION);
#ifdef PHP_EPEG_VERSION_STRING
	php_info_print_table_row(2, "Epeg Library Version", PHP_EPEG_VERSION_STRING);
#else
	php_info_print_table_row(2, "Epeg Library Version", "unknown");
#endif
	php_info_print_table_end();
}
/* }}} */

/* {{{ php_epeg_file_open */
static php_epeg_t *
php_epeg_file_open(char *file TSRMLS_DC)
{
	php_stream *sth = NULL;
	char *data = NULL;
	int data_len = 0;
	php_epeg_t *im = NULL;

	/* open stream for reading */
	sth = php_stream_open_wrapper(file, "rb",
			ENFORCE_SAFE_MODE | IGNORE_PATH | REPORT_ERRORS, NULL);
	if (!sth) {
		return NULL;
	}

	/* copy image data to the buffer */
	data_len = php_stream_copy_to_mem(sth, &data, PHP_STREAM_COPY_ALL, 0);

	/* close the input stream */
	php_stream_close(sth);
	if (data_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot read image data");
		return NULL;
	}

	/* open the JPEG image stored in the buffer */
	im = php_epeg_memory_open(data, data_len TSRMLS_CC);

	/* free the buffer */
	efree(data);

	/* return Epeg image handle (or NULL) */
	return im;
}
/* }}} */

/* {{{ php_epeg_memory_open */
static php_epeg_t *
php_epeg_memory_open(char *data, int data_len TSRMLS_DC)
{
	php_epeg_t *im = NULL;

	/* initialize */
	im = (php_epeg_t *)ecalloc(1, sizeof(php_epeg_t));
	im->data = (unsigned char *)estrndup(data, data_len);
	im->size = data_len;
	im->quality = -1;

	/* open the JPEG image stored in the buffer */
	im->ptr = epeg_memory_open(im->data, im->size);
	if (im->ptr == NULL) {
		php_epeg_free(im);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not a valid JPEG data");
		return NULL;
	}

	/* get image size */
	epeg_size_get(im->ptr, &(im->width), &(im->height));

	/* return Epeg image handle */
	return im;
}
/* }}} */

/* {{{ php_epeg_open_wrapper */
static void
php_epeg_open_wrapper(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
	/* declaration of the arguments */
	char *str = NULL;
	int str_len = 0;

	/* declaration of the local variables */
	php_epeg_t *im = NULL;

	/* parse the arguments and open the JPEG image */
	if (mode & EO_FROM_BUFFER) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &str, &str_len) == FAILURE) {
			RETURN_FALSE;
		}
		im = php_epeg_memory_open(str, str_len TSRMLS_CC);
	} else {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
			RETURN_FALSE;
		}
		im = php_epeg_file_open(str TSRMLS_CC);
	}
	if (im == NULL) {
		RETURN_FALSE;
	}

	if (mode & EO_TO_OBJECT) {
		php_epeg_object *intern;
		Z_TYPE_P(return_value) = IS_OBJECT;
		object_init_ex(return_value, ce_Epeg);
		intern = (php_epeg_object *)zend_object_store_get_object(return_value TSRMLS_CC);
		intern->ptr = im;
	} else {
		ZEND_REGISTER_RESOURCE(return_value, im, le_epeg);
	}
}
/* }}} */

/* {{{ php_epeg_set_retval */
static void
php_epeg_set_retval(unsigned char *buf, int buf_len,
		char *file, int file_len, zval *retval TSRMLS_DC)
{
	/* if the output is an empty string, the content of the thubmnail is returned */
	if (file_len == 0) {
		/* set return value to the content of the thumbnail */
		ZVAL_STRINGL(retval, (char *)buf, buf_len, 1);
	} else {
		/* open stream for writing */
		php_stream *sth = NULL;
		sth = php_stream_open_wrapper(file, "wb",
				ENFORCE_SAFE_MODE | IGNORE_PATH | REPORT_ERRORS, NULL);
		if (!sth) {
			/* set return value to false */
			ZVAL_FALSE(retval);
		} else {
			int expected_written_len = buf_len;
			if (expected_written_len != php_stream_write(sth, (char *)buf, buf_len)) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to write image data to stream");
				/* set return value to false */
				ZVAL_FALSE(retval);
			} else {
				/* set return value to true */
				ZVAL_TRUE(retval);
			}
			/* close the output stream */
			php_stream_close(sth);
		}
	}
}
/* }}} */

/* {{{ php_epeg_encode_error */
static void
php_epeg_encode_error(int errcode TSRMLS_DC)
{
	switch (errcode) {
	  case 3:
	  case 4:
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to decode image");
		break;
	  case 1:
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to scale image");
		break;
	  case 2:
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to encode image");
		break;
	  default:
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown error");
	}
}
/* }}} */

/* {{{ php_epeg_trim_error */
static void
php_epeg_trim_error(int errcode TSRMLS_DC)
{
	switch (errcode) {
	  case 1:
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to trim image");
		break;
	  default:
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown error");
	}
}
/* }}} */

/* {{{ php_epeg_free_resource */
static void
php_epeg_free_resource(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	php_epeg_free((php_epeg_t *)(rsrc->ptr));
}
/* }}} */

/* {{{ php_epeg_free */
static void
php_epeg_free(php_epeg_t *im)
{
	if (im->ptr != NULL) {
		epeg_close(im->ptr);
	}
	if (im->data != NULL) {
		efree(im->data);
	}
	efree(im);
}
/* }}} */

/* {{{ php_epeg_object_new */
static zend_object_value
php_epeg_object_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	php_epeg_object *intern;

	intern = (php_epeg_object *)ecalloc(1, sizeof(php_epeg_object));

	zend_object_std_init(&intern->std, ce TSRMLS_CC);
#if PHP_API_VERSION < 20100412
	zend_hash_copy(intern->std.properties, &ce->default_properties,
			(copy_ctor_func_t)zval_add_ref, NULL, sizeof(zval *));
#else
	object_properties_init(&intern->std, ce);
#endif

	retval.handle = zend_objects_store_put(intern,
			(zend_objects_store_dtor_t)zend_objects_destroy_object,
			(zend_objects_free_object_storage_t)php_epeg_free_object_storage,
			NULL TSRMLS_CC);
	retval.handlers = &_php_epeg_object_handlers;

	return retval;
}
/* }}} */

/* {{{ php_epeg_free_object_storage */
static void
php_epeg_free_object_storage(void *object TSRMLS_DC)
{
	php_epeg_object *intern = (php_epeg_object *)object;
	php_epeg_free(intern->ptr);
	zend_object_std_dtor(&intern->std TSRMLS_CC);
	efree(object);
}
/* }}} */

/* {{{ php_epeg_calc_thumb_size */
static int
php_epeg_calc_thumb_size(
		int src_width, int src_height,
		int max_width, int max_height,
		int *dst_width, int *dst_height)
{
	/* check */
	if (src_width < 1 || max_width < 0) {
		return FAILURE;
	} else if (max_width == 0) {
		max_width = src_width;
	}
	if (src_height < 1 || max_height < 0) {
		return FAILURE;
	} else if (max_height == 0) {
		max_height = src_height;
	}

	if (max_width >= src_width && max_height >= src_height) {
		*dst_width = src_width;
		*dst_height = src_height;
	} else {
		double width = (double)src_width;
		double height = (double)src_height;
		double to_width = (double)max_width;
		double to_height = (double)max_height;
		double xy_ratio = width / height;

		/* calculate */
		if (width <= to_width) {
			*dst_width = round_to_i(to_height * xy_ratio);
			*dst_height = round_to_i(to_height);
		} else if (height <= to_height) {
			*dst_width = round_to_i(to_width);
			*dst_height = round_to_i(to_width / xy_ratio);
		} else if (xy_ratio > to_width / to_height) {
			*dst_width = round_to_i(to_width);
			*dst_height = round_to_i(to_width / xy_ratio);
		} else {
			*dst_width = round_to_i(to_height * xy_ratio);
			*dst_height = round_to_i(to_height);
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_epeg_reset */
static void
php_epeg_reset(php_epeg_t *im)
{
	epeg_close(im->ptr);
	im->ptr = epeg_memory_open(im->data, im->size);

	if (im->quality != -1) {
		epeg_quality_set(im->ptr, im->quality);
	}
}
/* }}} */

/* {{{ proto mixed epeg_thumbnail_create(string in_file, string out_file, int max_width, int max_height[, int quality]) */
/**
 * bool|string epeg_thumbnail(string in_file, string out_file, int max_width, int max_height[, int quality])
 *
 * Create thumbnail using the Epeg library.
 * This function can be used for only JPEG image.
 *
 * @param	string	$in_file	The pathname or the URL of the source image.
 * @param	string	$out_file	The pathname or the URL of the thumbnail.
 * @param	int	$max_width	The maximum width of the thumbnail.
 *							The value must be greater than 0.
 * @param	int	$max_height	The maximum height of the thumbnail.
 *							The value must be greater than 0.
 * @param	int	$quality	The quality of the thumbnail. (optional)
 *							The value must be greater than or equal to 0
 *							and must be less than or equal to 100.
 *							The default is 75.
 * @return	mixed	False is returned if failed to create the thumbnail.
 *					True is returned if succeeded in creating and writing the thumbnail.
 *					If $out_file is an empty string and succeeded in creating
 *					the thumbnail, the content of the thumbnail is returned.
 */
static PHP_FUNCTION(epeg_thumbnail_create)
{
	/* declaration of the arguments */
	char *in_file = NULL;
	int in_file_len = 0;
	char *out_file = NULL;
	int out_file_len = 0;
	long max_width = 0;
	long max_height = 0;
	long quality = 75;

	/* declaration of the local variables */
	php_epeg_t *im = NULL;
	php_stream *sth = NULL;
	char *in_buf;
	unsigned char *out_buf;
	int in_buf_len, out_buf_len;
	zend_bool out_buf_free_stdc = 0; /* use free() */
	zend_bool out_buf_free_zend = 0; /* use efree() */

	/* parse the arguments */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssll|l",
			&in_file, &in_file_len, &out_file, &out_file_len,
			&max_width, &max_height, &quality) == FAILURE)
	{
		RETURN_FALSE;
	}

	/* check output size */
	if (max_width <= 0 || max_height <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
				"Invalid image dimensions '%ldx%ld'", max_width, max_height);
		RETURN_FALSE;
	}

	/* check quality */
	if (quality < 0 || quality > 100) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid quality '%ld'", quality);
		RETURN_FALSE;
	}

	/* open stream for reading */
	sth = php_stream_open_wrapper(in_file, "rb",
			ENFORCE_SAFE_MODE | IGNORE_PATH | REPORT_ERRORS, NULL);
	if (!sth) {
		RETURN_FALSE;
	}

	/* copy image data to the buffer */
	in_buf_len = php_stream_copy_to_mem(sth, &in_buf, PHP_STREAM_COPY_ALL, 0);

	/* close the input stream */
	php_stream_close(sth);
	if (in_buf_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot read image data");
		RETURN_FALSE;
	}

	/* open the JPEG image stored in the buffer */
	im = php_epeg_memory_open(in_buf, in_buf_len TSRMLS_CC);
	if (im == NULL) {
		/* free the input buffer */
		efree(in_buf);
		RETURN_FALSE;
	}

	/* get image size and check whether to do resampling */
	if (im->width > max_width || im->height > max_height) {
		unsigned char *tmp_buf;
		int result, tw, th, tmp_buf_len;

		/* free the input buffer */
		efree(in_buf);
		/* calculate size */
		(void)php_epeg_calc_thumb_size(im->width, im->height, max_width, max_height, &tw, &th);
		/* set the size of thumbnail */
		epeg_decode_size_set(im->ptr, tw, th);
		/* set output to the buffer */
		epeg_memory_output_set(im->ptr, &tmp_buf, &tmp_buf_len);
		/* set quality */
		epeg_quality_set(im->ptr, (int)quality);
		/* encode the image and save to the buffer */
		result = epeg_encode(im->ptr);
		/* close the Epeg image handle */
		php_epeg_free(im);
		if (result != 0) {
			/* free the temporary buffer */
			if (tmp_buf) {
				free(tmp_buf);
			}
			/* raise error by the result */
			php_epeg_encode_error(result TSRMLS_CC);
			RETURN_FALSE;
		}

		/* set the result */
		out_buf = tmp_buf;
		out_buf_len = tmp_buf_len;
		out_buf_free_stdc = 1;
	} else {
		unsigned char *in_ptr, *in_end, *out_ptr;

		/* close the Epeg image handle */
		php_epeg_free(im);

		/* cast the input data to unsigned char */
		in_ptr = (unsigned char *)in_buf;
		in_end = in_ptr + in_buf_len;

		/* check for SOI (Start Of Image Segment) marker */
		if (*in_ptr != 0xFF && *(in_ptr + 1) != 0xD8) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not a valid JPEG data");
			efree(in_buf);
			RETURN_FALSE;
		}

		/* allocate memory for the output buffer */
		out_buf = (unsigned char *)emalloc((size_t)in_buf_len);
		out_ptr = out_buf;
		out_buf_free_zend = 1;

		/* write SOI marker  */
		*out_ptr++ = 0xFF;
		*out_ptr++ = 0xD8;
		in_ptr += 2;

		/* search and skip extra markers */
		while (in_ptr < in_end) {
			unsigned char marker;
			size_t field_len;

			if (*in_ptr == 0xFF) {
				/* skip padding */
				while (*in_ptr == 0xFF && in_ptr < in_end) {
					in_ptr++;
				}
				marker = *in_ptr++;
				field_len = (((size_t)*in_ptr) << 8) | (size_t)*(in_ptr + 1);

				if (in_ptr + field_len > in_end) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid data structure");
					efree(out_buf);
					RETURN_FALSE;
				}

				/* following markers are dropped :
				 * RES:        0xFF [0x02-0xBF] (reserved)
				 * APP[1-15]:  0xFF [0xE1-0xEF] (application markers, APP0 (0xE0) is JFIF (kept), APP1 is EXIF)
				 * JPEG[0-13]: 0xFF [0xF0-0xFD] (reserved for expansion of JPEG)
				 * COM:        0xFF 0xFE (comment)
				 */
				if ((marker > 0x01 && marker < 0xC0) || (marker > 0xE0 && marker < 0xFF)) {
					in_ptr += field_len;
					continue;
				} else {
					*out_ptr++ = 0xFF;
					*out_ptr++ = marker;
					(void)memcpy(out_ptr, in_ptr, field_len);
					in_ptr += field_len;
					out_ptr += field_len;

					/* break if SOS (Start Of Scan) marker found */
					if (marker == 0xDA) {
						break;
					}
				}
			} else {
				*out_ptr++ = *in_ptr++;
			}
		}

		/* copy leftovers */
		if (in_ptr < in_end) {
			size_t rest_len = (size_t)(in_end - in_ptr);
			(void)memcpy(out_ptr, in_ptr, rest_len);
			out_ptr += rest_len;
		}

		/* free the input buffer */
		efree(in_buf);

		/* set the result */
		out_buf_len = (int)(out_ptr - out_buf);
	}

	/* set return value */
	php_epeg_set_retval(out_buf, out_buf_len, out_file, out_file_len, return_value TSRMLS_CC);

	/* free the output buffer */
	if (out_buf_free_stdc) {
		free(out_buf);
	} else if (out_buf_free_zend) {
		efree(out_buf);
	}
}
/* }}} epeg_thumbnail_create */

/* {{{ proto resource epeg_open(string filename[, boolean is_data]) */
/**
 * resource epeg epeg_open(string filename[, boolean is_data])
 * object Epeg Epeg::__construct(string filename[, boolean is_data])
 *
 * Open a JPEG image for thumbnailing.
 *
 * @param	string	$filename	The pathname or the URL or the binary data of the source image.
 * @param	bool	$is_data	Whether $file is a binary data or not.
 * @return	resource epeg	An Epeg image handle is returned if succeeded in opening the image.
 *							False is returned if failed to open the image.
 */
static PHP_FUNCTION(epeg_open)
{
	/* declaration of the resources */
	zval *obj = getThis();
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	char *file = NULL;
	int file_len = 0;
	zend_bool is_data = 0;

	/* parse arguments */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &file, &file_len, &is_data) == FAILURE) {
		RETURN_FALSE;
	}

	if (is_data) {
		/* open the JPEG image stored in the buffer */
		im = php_epeg_memory_open(file, file_len TSRMLS_CC);
	} else {
		/* open Epeg image handle */
		im = php_epeg_file_open(file TSRMLS_CC);
	}
	if (im == NULL) {
		RETURN_FALSE;
	}

	if (obj) {
		php_epeg_object *intern;
		intern = (php_epeg_object *)zend_object_store_get_object(obj TSRMLS_CC);
		if (intern->ptr != NULL) {
			php_epeg_free(im);
			zend_throw_exception(zend_exception_get_default(TSRMLS_C),
					"Epeg already initialized", 0 TSRMLS_CC);
			return;
		}
		intern->ptr = im;
	} else {
		ZEND_REGISTER_RESOURCE(return_value, im, le_epeg);
	}
}
/* }}} epeg_open */

/* {{{ proto resource epeg epeg_file_open(string filename) */
/**
 * resource epeg epeg_file_open(string filename)
 *
 * Open a JPEG image for thumbnailing from pathname or URL.
 *
 * @param	string	$filename	The pathname or the URL of the source image.
 * @return	resource epeg	An Epeg image handle is returned if succeeded in opening the image.
 *							False is returned if failed to open the image.
 */
static PHP_FUNCTION(epeg_file_open)
{
	php_epeg_open_wrapper(INTERNAL_FUNCTION_PARAM_PASSTHRU, EO_FROM_FILE | EO_TO_RESOURCE);
}
/* }}} epeg_file_open */

/* {{{ proto resource epeg epeg_memory_open(string data) */
/**
 * resource epeg epeg_memory_open(string data)
 *
 * Open a JPEG image for thumbnailing from string.
 *
 * @param	string	$data	The binary data of the source image.
 * @return	resource epeg	An Epeg image handle is returned if succeeded in opening the image.
 *							False is returned if failed to open the image.
 */
static PHP_FUNCTION(epeg_memory_open)
{
	php_epeg_open_wrapper(INTERNAL_FUNCTION_PARAM_PASSTHRU, EO_FROM_BUFFER | EO_TO_RESOURCE);
}
/* }}} epeg_memory_open */

/* {{{ proto object Epeg Epeg::openFile(string filename) */
/**
 * object Epeg Epeg::openFile(string filename)
 *
 * Open a JPEG image for thumbnailing from pathname or URL.
 *
 * @param	string	$filename	The pathname or the URL of the source image.
 * @return	object Epeg	An instance of class Epeg is returned if succeeded in opening the image.
 *							False is returned if failed to open the image.
 * @access public
 * @static
 */
PHP_METHOD(Epeg, openFile)
{
	php_epeg_open_wrapper(INTERNAL_FUNCTION_PARAM_PASSTHRU, EO_FROM_FILE | EO_TO_OBJECT);
}
/* }}} Epeg::openFile */

/* {{{ proto object Epeg Epeg::openBuffer(string data) */
/**
 * object Epeg Epeg::openBuffer(string data)
 *
 * Open a JPEG image for thumbnailing from string.
 *
 * @param	string	$data	The binary data of the source image.
 * @return	object Epeg	An instance of class Epeg is returned if succeeded in opening the image.
 *							False is returned if failed to open the image.
 * @access public
 * @static
 */
PHP_METHOD(Epeg, openBuffer)
{
	php_epeg_open_wrapper(INTERNAL_FUNCTION_PARAM_PASSTHRU, EO_FROM_BUFFER | EO_TO_OBJECT);
}
/* }}} Epeg::openBuffer */

/* {{{ proto array epeg_size_get(resource epeg image) */
/**
 * array epeg_size_get(resource epeg image)
 * array Epeg::getSize(void)
 *
 * Get the size of the source image.
 * The width is stored in both offset 0 and index "width" of return value.
 * The height is stored in both offset 1 and index "height" of return value.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @return	array	The size of the image.
 */
static PHP_FUNCTION(epeg_size_get)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETER();

	/* initialize return_value as an array */
	array_init(return_value);

	/* set return value to the width and the height */
	add_index_long(return_value, 0, (long)im->width);
	add_index_long(return_value, 1, (long)im->height);
	add_assoc_long(return_value, "width", (long)im->width);
	add_assoc_long(return_value, "height", (long)im->height);
}
/* }}} epeg_size_get */

/* {{{ proto void epeg_decode_size_set(resource epeg image, int width, int height[, bool keep_aspect]) */
/**
 * void epeg_decode_size_set(resource epeg image, int width, int height[, bool keep_aspect])
 * void Epeg::setDecodeSize(int width, int height[, bool keep_aspect])
 *
 * Set the size of the thumbnail.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	int	$width	The width of the thumbnail.
 *						The value must be greater than 0.
 * @param	int	$height	The height of the thumbnail.
 *						The value must be greater than 0.
 * @param	bool	$keep_aspect	Whether to keep the aspect ratio.
 *						The default is false.
 * @return	void
 */
static PHP_FUNCTION(epeg_decode_size_set)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	long w = 0;
	long h = 0;
	zend_bool keep_aspect = 0;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("ll|b", &w, &h, &keep_aspect);

	/* check decode size */
	if (w <= 0 || h <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid image dimensions");
		return;
	}

	/* set decode size */
	if (keep_aspect) {
		int tw = 0, th = 0;
		(void)php_epeg_calc_thumb_size(im->width, im->height, (int)w, (int)h, &tw, &th);
		epeg_decode_size_set(im->ptr, tw, th);
	} else {
		epeg_decode_size_set(im->ptr, (int)w, (int)h);
	}
}
/* }}} epeg_decode_size_set */

#ifdef PHP_EPEG_ENABLE_DECODE_BOUNDS_SET
/* {{{ proto void epeg_decode_bounds_set(resource epeg image, int x, int y, int width, int height) */
/**
 * void epeg_decode_bounds_set(resource epeg image, int x, int y, int width, int height)
 * void Epeg::setDecodeBounds(int x, int y, int width, int height)
 *
 * Set the bounds of the thumbnail.
 * Usually used with epeg_trim().
 *
 * $x and $y must not be less than 0,
 * $width and $height must be greater than 0,
 * ($x + $width) and ($y + $height) must not be greater than the original.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	int	$x	The horizontal start position of the source image.
 * @param	int	$y	The vertical start position of the source image.
 * @param	int	$width	The horizontal distance from the start position.
 * @param	int	$height	The vertical distance from the start position.
 * @return	void
 */
static PHP_FUNCTION(epeg_decode_bounds_set)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	long x = 0;
	long y = 0;
	long w = 0;
	long h = 0;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("llll", &x, &y, &w, &h);

	/* check decode bounds */
	if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > (long)im->width || y + h > (long)im->height) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid image dimensions");
		return;
	}

	/* set decode bounds */
	epeg_decode_bounds_set(im->ptr, (int)x, (int)y, (int)w, (int)h);
}
/* }}} epeg_decode_bounds_set */
#endif

/* {{{ proto void epeg_decode_colorspace_set(resource epeg image, int colorspace) */
/**
 * void epeg_decode_colorspace_set(resource epeg image, int colorspace)
 * void Epeg::getSize(int colorspace)
 *
 * Set the colorspace of the thumbnail.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	int	$colorspace	The colorspace of the thumbnail.
 *							The value must be one of the following:
 *								EPEG_GRAY8
 *								EPEG_YUV8
 *								EPEG_RGB8
 *								EPEG_BGR8
 *								EPEG_RGBA8
 *								EPEG_BGRA8
 *								EPEG_ARGB32
 *								EPEG_CMYK
 * @return	void
 */
static PHP_FUNCTION(epeg_decode_colorspace_set)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	long colorspace = 0;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("l", &colorspace);

	/* check colorspace */
	if (colorspace < (long)EPEG_GRAY8 || colorspace > (long)EPEG_CMYK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid colorspace");
	} else {
		epeg_decode_colorspace_set(im->ptr, (Epeg_Colorspace)colorspace);
	}
}
/* }}} epeg_decode_colorspace_set */

/* {{{ proto string epeg_comment_get(resource epeg image) */
/**
 * string epeg_comment_get(resource epeg image)
 * string Epeg::getComment(void)
 *
 * Get the comment field of the source image.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @return	string	The comment field of the source image.
 */
static PHP_FUNCTION(epeg_comment_get)
{
	/* declaration of the arguments */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the local variables */
	const char *comment = NULL;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETER();

	/* get comment */
	comment = epeg_comment_get(im->ptr);
	if (comment == NULL) {
		RETURN_EMPTY_STRING();
	} else {
		RETURN_STRING((char *)comment, 1);
	}
}
/* }}} epeg_comment_get */

/* {{{ proto void epeg_comment_set(resource epeg image, string comment) */
/**
 * void epeg_comment_set(resource epeg image, string comment)
 * void Epeg::setComment(string comment)
 *
 * Set the comment field of the thumbnail.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	string	$comment	The comment field of the thumbnail.
 * @return	void
 */
static PHP_FUNCTION(epeg_comment_set)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	const char *comment = NULL;
	int comment_len = 0;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("s", &comment, &comment_len);

	/* set the comment */
	epeg_comment_set(im->ptr, comment);
}
/* }}} epeg_comment_set */

/* {{{ proto void epeg_quality_set(resource epeg image, int quality) */
/**
 * void epeg_quality_set(resource epeg image, int quality)
 * void Epeg::setQuality(int quality)
 *
 * Set the quality of the thumbnail.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	int	$quality	The quality of the thumbnail. (optional)
 *							The value must be greater than or equal to 0
 *							and must be less than or equal to 100.
 *							The default is 75.
 * @return	void
 */
static PHP_FUNCTION(epeg_quality_set)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	long quality = 0;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("l", &quality);

	/* check quality */
	if (quality < 0 || quality > 100L) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid quality (%ld)", quality);
		return;
	}

	/* set the quality */
	im->quality = (int)quality;
	epeg_quality_set(im->ptr, im->quality);
}
/* }}} epeg_quality_set */

/* {{{ proto array epeg_thumbnail_comments_get(resource epeg image) */
/**
 * array epeg_thumbnail_comments_get(resource epeg image)
 * array Epeg::getThumbnailComments(void)
 *
 * Get thumbnail comments of loaded image.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @return	array	Thumbnail comments written by Epeg to any saved JPEG files.
 */
static PHP_FUNCTION(epeg_thumbnail_comments_get)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the local variables */
	Epeg_Thumbnail_Info info = {0};

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETER();

	/* get thumbnail comments */
	epeg_thumbnail_comments_get(im->ptr, &info);

	/* initialize return_value as an array */
	array_init(return_value);

	/* set return value to the width and the height */
	if (info.uri == NULL) {
		add_assoc_null(return_value, "uri");
	} else {
		add_assoc_string(return_value, "uri", (char *)info.uri, 1);
	}
	add_assoc_long(return_value, "mtime", (long)info.mtime);
	add_assoc_long(return_value, "width", (long)info.w);
	add_assoc_long(return_value, "height", (long)info.h);
	if (info.mimetype == NULL) {
		add_assoc_null(return_value, "mimetype");
	} else {
		add_assoc_string(return_value, "mimetype", (char *)info.mimetype, 1);
	}
}
/* }}} epeg_thumbnail_comments_get */

/* {{{ proto void epeg_thumbnail_comments_enable(resource epeg image, bool onoff) */
/**
 * void epeg_thumbnail_comments_enable(resource epeg image, bool onoff)
 * void Epeg::enableThumbnailComments(bool onoff)
 *
 * Enable or disable thumbnail comments in saved image.
 * The default is false (disabled).
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	bool	$onoff	A boolean on and off enabling flag.
 * @return	void
 */
static PHP_FUNCTION(epeg_thumbnail_comments_enable)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	zend_bool onoff = 1;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("|b", &onoff);

	/* enable/disable thumbnail comments */
	epeg_thumbnail_comments_enable(im->ptr, (int)onoff);
}
/* }}} epeg_thumbnail_comments_enable */

/* {{{ proto mixed epeg_encode(resource epeg image[, string filename]) */
/**
 * mixed epeg_encode(resource epeg image[, string filename])
 * mixed Epeg::encode([string filename])
 *
 * Save or get the scaled image.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	string	$filename	The pathname or the URL of the source image. (optional)
 * @return	bool|string	False is returned if failed to create the thumbnail.
 *						True is returned if succeeded in creating and writing the thumbnail.
 *						If $out_file is an empty string and succeeded in creating
 *						the thumbnail, the content of the thumbnail is returned.
 */
static PHP_FUNCTION(epeg_encode)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	char *file = NULL;
	int file_len = 0;

	/* declaration of the local variables */
	unsigned char *buf = NULL;
	int buf_len = 0;
	int result = 0;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("|s", &file, &file_len);

	/* set output to the buffer */
	epeg_memory_output_set(im->ptr, &buf, &buf_len);

	/* encode the image */
	if ((result = epeg_encode(im->ptr)) != 0) {
		/* free the buffer */
		if (buf) {
			free(buf);
		}
		/* raise error by the result */
		php_epeg_encode_error(result TSRMLS_CC);
		RETURN_FALSE;
	}

	/* set return value */
	php_epeg_set_retval(buf, buf_len, file, file_len, return_value TSRMLS_CC);

	/* free the buffer */
	free(buf);

	/* reset internal image handler */
	php_epeg_reset(im);
}
/* }}} epeg_encode */

/* {{{ proto mixed epeg_trim(resource epeg image[, string filename]) */
/**
 * mixed epeg_trim(resource epeg image[, string filename])
 * mixed Epeg::trim([string filename])
 *
 * Save or get the trimed image.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @param	string	$filename	The pathname or the URL of the source image. (optional)
 * @return	bool|string	False is returned if failed to create the thumbnail.
 *						True is returned if succeeded in creating and writing the thumbnail.
 *						If $out_file is an empty string and succeeded in creating
 *						the thumbnail, the content of the thumbnail is returned.
 */
static PHP_FUNCTION(epeg_trim)
{
	/* declaration of the resources */
	zval *obj = getThis();
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* declaration of the arguments */
	char *file = NULL;
	int file_len = 0;

	/* declaration of the local variables */
	unsigned char *buf = NULL;
	int buf_len = 0;
	int result = 0;

	/* parse the arguments */
	PHP_EPEG_PARSE_PARAMETERS("|s", &file, &file_len);

	/* set output to the buffer */
	epeg_memory_output_set(im->ptr, &buf, &buf_len);

	/* trim the image */
	if ((result = epeg_trim(im->ptr)) != 0) {
		/* free the buffer */
		if (buf) {
			free(buf);
		}
		/* raise error by the result */
		php_epeg_trim_error(result TSRMLS_CC);
		RETURN_FALSE;
	}

	/* set return value */
	php_epeg_set_retval(buf, buf_len, file, file_len, return_value TSRMLS_CC);

	/* free the buffer */
	free(buf);

	/* reset internal image handler */
	php_epeg_reset(im);
}
/* }}} epeg_trim */

/* {{{ proto void epeg_close(resource epeg image) */
/**
 * void epeg_close(resource epeg image)
 * void Epeg::__destruct()
 *
 * Free the Epeg image handle.
 *
 * @param	resource epeg	$image	An Epeg image handle.
 * @return	void
 */
static PHP_FUNCTION(epeg_close)
{
	/* declaration of the arguments */
	zval *zim = NULL;
	php_epeg_t *im = NULL;

	/* parse the arguments */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zim) == FAILURE) {
		return;
	}
	FETCH_IMAGE_FROM_RESOURCE(im, zim);

	/* free the resource */
	FREE_RESOURCE(zim);
 }
/* }}} epeg_close */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
