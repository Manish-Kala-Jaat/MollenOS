/* MollenOS
 *
 * Copyright 2011 - 2017, Philip Meulengracht
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ? , either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * MollenOS - C Standard Library
 * - Writes the C string pointed by str to the stream.
 * - The function begins copying from the address specified (str) until it 
 *   reaches the terminating null character ('\0'). 
 *   This terminating null-character is not copied to the stream.
 */

#include <wchar.h>
#include <stdio.h>
#include "local.h"

int fputws(
    _In_ __CONST wchar_t *s, 
    _In_ FILE* file)
{
    size_t i, len = wcslen(s);
    int tmp_buf = 0;
    int ret;

    _lock_file(file);
    if (!(get_ioinfo(file->_fd)->wxflag & WX_TEXT)) {
        ret = fwrite(s,sizeof(*s),len,file) == len ? 0 : EOF;
        _unlock_file(file);
        return ret;
    }

    tmp_buf = add_std_buffer(file);
    for (i=0; i<len; i++) {
        if(fputwc(s[i], file) == WEOF) {
            if(tmp_buf) remove_std_buffer(file);
            _unlock_file(file);
            return WEOF;
        }
    }

    if(tmp_buf) remove_std_buffer(file);
    _unlock_file(file);
    return 0;
}
