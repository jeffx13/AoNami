import os,re
classFile = "providersmodel"
newName = "ProvidersModel"
className = None
path = os.path.join(os.path.dirname(__file__),f'{classFile}')
def getClassName():
    global className
    with open(f'{path}.h','r') as f:
        text = f.read()
        className = re.search(r'(?<=class )\w+',text).group(0)
        print(className)
     
def changeName(filePath):
    global newName,className
    if not className:
        getClassName()
    with open(filePath,"r") as f:
        replacement = f.read().replace(className,newName).replace(className.upper(),newName.upper())
    with open(filePath,"w") as f:
        f.write(replacement)
        
        
     
    
if os.path.exists(f'{path}.h'):
    changeName(f'{path}.h')
if os.path.exists(f'{path}.cpp'):
    changeName(f'{path}.cpp')    
