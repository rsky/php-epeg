/**
 * The Epeg PHP extension
 *
 * Copyright (c) 2006-2009 Ryusuke SEKIYAMA. All rights reserved.
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
 * @package     php_epeg
 * @author      Ryusuke SEKIYAMA <rsky0711@gmail.com>
 * @copyright   2006-2009 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 * @version     SVN: $Id$
 */

#ifndef _PHP_EPEG_H_
#define _PHP_EPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <php_ini.h>
#include <SAPI.h>
#include <ext/standard/info.h>
#include <Zend/zend_extensions.h>
#ifdef ZEND_ENGINE_2
#include <Zend/zend_exceptions.h>
#endif
#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef __cplusplus
} // extern "C"
#endif
#include <math.h>
#include <Epeg.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PHP_EPEG_MODULE_VERSION "0.2.1"

#define EO_FROM_FILE    (1 << 0)
#define EO_FROM_BUFFER  (1 << 1)
#define EO_TO_RESOURCE  (1 << 2)
#define EO_TO_OBJECT    (1 << 3)

#ifdef PHP_EPEG_ENABLE_DECODE_BOUNDS_SET
void epeg_decode_bounds_set(Epeg_Image *im, int x, int y, int w, int h);
#endif

/* {{{ type definitions */

typedef struct _php_epeg_t {
	Epeg_Image *ptr;
	unsigned char *data;
	int size;
	int width;
	int height;
	int quality;
} php_epeg_t;

#ifdef ZEND_ENGINE_2
typedef struct _php_epeg_object {
	zend_object std;
	php_epeg_t *ptr;
} php_epeg_object;
#endif

/* }}} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _PHP_EPEG_H_ */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
