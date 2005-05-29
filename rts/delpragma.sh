#!/bin/bash

for FILE in $@;
do
  if grep '#pragma once' $FILE
  then
   echo $FILE
   echo -n "#ifndef " > "$FILE"_ren_tmp
   echo "$FILE" | sed -r "s/./\U&/g" | sed -r "s/[.]/_/g" >> "$FILE"_ren_tmp
   echo -n "#define " >> "$FILE"_ren_tmp
   echo "$FILE" | sed -r "s/./\U&/g" | sed -r "s/[.]/_/g" >> "$FILE"_ren_tmp

   sed 's/#pragma once/\/*pragma once removed*\//g' $FILE >> "$FILE"_ren_tmp

   echo '' >> "$FILE"_ren_tmp
   echo -n "#endif " >> "$FILE"_ren_tmp
   echo "/* $FILE */" | sed -r "s/./\U&/g" | sed -r "s/[.]/_/g" >> "$FILE"_ren_tmp

   mv "$FILE"_ren_tmp $FILE
  fi
done
