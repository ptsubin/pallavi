#!/bin/bash
#
# generate_lm.sh
#
# Copyright 2014 Pallavi Project <pallavi.malayalam@gmail.com>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
#

status ()

	if [ $1 -eq 0 ] ; then
		echo "[-------------------- SUCCESS ------------------]"
		echo ""
	else
		echo "[-------------------- FAILED -------------------]"
		if [ $2 -eq 1 ] ; then
			echo "Command failed. Aborting.."
			exit 1
		fi
	fi


if [ $# -ne 2 ]; then
	echo "Usage is $0 <transcript file> <model name>"
	exit 1
fi

echo "<s>\n</s>" > $2.ccs

echo "Step 1, running text2wfreq.."
cat $1 | text2wfreq > $2.wfreq
status $? 1

echo "Step 2, running wfreq2vocab.."
cat $2.wfreq | wfreq2vocab > $2.vocab
status $? 1

echo "Step 3, running text2idngram.."
cat $1 | text2idngram -n 3 -vocab $2.vocab -temp /tmp > $2.idngram
status $? 1

echo "Step 4, running idngram2lm.."
idngram2lm -n 3 -vocab $2.vocab -idngram $2.idngram -arpa $2.lm -context $2.ccs
status $? 1

echo "Step 5, running sphinx3_lm_convert.."
sphinx3_lm_convert -i $2.lm -o $2.lm.DMP
status $? 1

echo "Done.."

