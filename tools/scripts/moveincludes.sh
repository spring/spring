#!/bin/sh

#changes include path in all files
#has to be executed in rts/

#search all header files in System/
for i in $(find System -type f -not -regex "./lib.*" -and -name *.h);
do
	escaped=${i//\//\\/}
	old_filename=${escaped/System\\\//} #remove System prefix
	new_filename="${escaped}"
	exclude_dir=$(dirname $i)
	echo "$old_filename -> $new_filename (exclude $exclude_dir)"
	#in all files replace occurences of old_file to new_file, excluding dir whre old_file is
	find  -type f -not -regex "./lib.*" -not -regex "${exclude_dir}.*"  -and  \( -name "*.cpp" -or -name "*.h" -or -name "*.hpp" \) |xargs sed -i "s/#include \"${old_filename}\"/#include \"${new_filename}\"/"
done

