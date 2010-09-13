import os
import re

#==[regexps]=================================================

# Clean desc 
REdefine = re.compile(r''
                r'(?P<desc>)'                                       # /** *desc */
                r'#\s*define\s(?P<name>[^(\n]+?)\s(?P<code>.+)$'    #  #define name value
                , re.MULTILINE)

# Get structs
REstructs = re.compile(r''
                #r'//\s?[\-]*\s(?P<desc>.*?)\*/\s'           # /** *desc */
                #r'//\s?[\-]*(?P<desc>.*?)\*/(?:.*?)'            # garbage
                r'//\s?[\-]*\s(?P<desc>.*?)\*/\W*?'           # /** *desc */
                r'struct\s(?:ASSIMP_API\s)?(?P<name>[a-z][a-z0-9_]\w+\b)'    # struct name
                r'[^{]*?\{'                                 # {
                r'(?P<code>.*?)'                            #   code 
                r'\}\s*(PACK_STRUCT)?;'                      # };
                , re.IGNORECASE + re.DOTALL + re.MULTILINE)

# Clean desc 
REdesc = re.compile(r''
                r'^\s*?([*]|/\*\*)(?P<line>.*?)'            #  * line 
                , re.IGNORECASE + re.DOTALL + re.MULTILINE)

# Remove #ifdef __cplusplus   
RErmifdef = re.compile(r''
                r'#ifdef __cplusplus'                       # #ifdef __cplusplus
                r'(?P<code>.*)'                             #   code 
                r'#endif(\s*//\s*!?\s*__cplusplus)*'          # #endif
                , re.IGNORECASE + re.DOTALL)
                
# Replace comments
RErpcom = re.compile(r''
                r'[ ]*(/\*\*\s|\*/|\B\*\s|//!)'             # /**
                r'(?P<line>.*?)'                            #  * line 
                , re.IGNORECASE + re.DOTALL)
                
# Restructure 
def GetType(type, prefix='c_'):
    t = type
    while t.endswith('*'):
        t = t[:-1]
    types = {'unsigned int':'uint', 'unsigned char':'ubyte', 'size_t':'uint',}
    if t in types:
        t = types[t]
    t = prefix + t
    while type.endswith('*'):
        t = "POINTER(" + t + ")"
        type = type[:-1]
    return t
    
def restructure( match ):
    type = match.group("type")
    if match.group("struct") == "":
        type = GetType(type)
    elif match.group("struct") == "C_ENUM ":
        type = "c_uint"
    else:
        type = GetType(type[2:], '')
    if match.group("index"):
        type = type + "*" + match.group("index")
        
    result = ""
    for name in match.group("name").split(','):
        result += "(\"" + name.strip() + "\", "+ type + "),"
    
    return result

RErestruc = re.compile(r''
                r'(?P<struct>C_STRUCT\s|C_ENUM\s|)'                     #  [C_STRUCT]
                r'(?P<type>\w+\s?\w+?[*]*)\s'                           #  type
                #r'(?P<name>\w+)'                                       #  name
                r'(?P<name>\w+|[a-z0-9_, ]+)'                               #  name
                r'(:?\[(?P<index>\w+)\])?;'                             #  []; (optional)
                , re.DOTALL)
#==[template]================================================
template = """
class $NAME$(Structure):
    \"\"\"
$DESCRIPTION$
    \"\"\" 
$DEFINES$
    _fields_ = [
            $FIELDS$
        ]
"""

templateSR = """
class $NAME$(Structure):
    \"\"\"
$DESCRIPTION$
    \"\"\" 
$DEFINES$

$NAME$._fields_ = [
            $FIELDS$
        ]
"""
#============================================================
def Structify(fileName):
    file = open(fileName, 'r')
    text = file.read()
    result = []
    
    # Get defines.
    defs = REdefine.findall(text)
    # Create defines
    defines = "\n"
    for define in defs:
        # Clean desc 
        desc = REdesc.sub('', define[0])
        # Replace comments
        desc = RErpcom.sub('#\g<line>', desc)
        defines += desc
        defines += " "*4 + define[1] + " = " + define[2] + "\n"  
    
    # Get structs
    rs = REstructs.finditer(text)

    fileName = os.path.basename(fileName)
    print fileName
    for r in rs:
        name = r.group('name')[2:]
        desc = r.group('desc')
        
        # Skip some structs
        if name == "FileIO" or name == "File" or name == "locateFromAssimpHeap":
            continue

        text = r.group('code')

        # Clean desc 
        desc = REdesc.sub('', desc)
        
        desc = "See '"+ fileName +"' for details." #TODO  
        
        # Remove #ifdef __cplusplus
        text = RErmifdef.sub('', text)
        
        # Whether the struct contains more than just POD
        primitive = text.find('C_STRUCT') == -1

        # Restructure
        text = RErestruc.sub(restructure, text)

        # Replace comments
        text = RErpcom.sub('#\g<line>', text)
        
        # Whether it's selfreferencing: ex. struct Node { Node* parent; };
        selfreferencing = text.find('POINTER('+name+')') != -1
        
        complex = name == "Scene"
        
        # Create description
        description = ""
        for line in desc.split('\n'):
            description += " "*4 + line.strip() + "\n"  
        description = description.rstrip()

        # Create fields
        fields = ""
        for line in text.split('\n'):
            fields += " "*12 + line.strip() + "\n"
        fields = fields.strip()

        if selfreferencing:
            templ = templateSR
        else:
            templ = template
            
        # Put it all together
        text = templ.replace('$NAME$', name)
        text = text.replace('$DESCRIPTION$', description)
        text = text.replace('$FIELDS$', fields)
        
        if ((name.lower() == fileName.split('.')[0][2:].lower()) and (name != 'Material')) or name == "String":
            text = text.replace('$DEFINES$', defines)
        else:
            text = text.replace('$DEFINES$', '')
        
        result.append((primitive, selfreferencing, complex, text))
             
    return result   

text = "#-*- coding: UTF-8 -*-\n\n"
text += "from ctypes import POINTER, c_int, c_uint, c_char, c_float, Structure, c_char_p, c_double, c_ubyte\n\n"

structs1 = ""
structs2 = ""
structs3 = ""
structs4 = ""

path = '../include/'
files = os.listdir (path)
#files = ["aiScene.h", "aiTypes.h"]
for fileName in files:
    if fileName.endswith('.h'):
        for struct in Structify(os.path.join(path, fileName)):
            primitive, sr, complex, struct = struct
            if primitive:
                structs1 += struct
            elif sr:
                structs2 += struct
            elif complex:
                structs4 += struct
            else:
                structs3 += struct

text += structs1 + structs2 + structs3 + structs4

file = open('structs.txt', 'w')
file.write(text)
file.close()
