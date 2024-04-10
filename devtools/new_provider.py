import os

try:
    provider_name = input("Enter Provider Name: ")
    outputPath = os.path.join(os.path.dirname(__file__),"..\src\Providers",provider_name.lower())
    with open(os.path.join(os.path.dirname(__file__),"template.h"),"r") as f:
        template_h = f.read().replace("NewProvider",provider_name)
    with open(os.path.join(os.path.dirname(__file__),"template.cpp"),"r") as f:
        template_cpp = f.read().replace('"template.h"',f'"{provider_name.lower()}.h"').replace("NewProvider",provider_name).replace("NEWPROVIDER",provider_name.upper())
    with open(f"{outputPath}.h","w") as f:
        f.write(template_h)
        print(f"created {outputPath}.h")
    with open(f"{outputPath}.cpp","w") as f:
        f.write(template_cpp)
        print(f"created {outputPath}.cpp")
    input("operation success!")
except KeyboardInterrupt:
    if provider_name:
        if os.path.exists(f"{outputPath}.h"):
            os.remove(f"{outputPath}.h")
        if os.path.exists(f"{outputPath}.cpp"):
            os.remove(f"{outputPath}.cpp")