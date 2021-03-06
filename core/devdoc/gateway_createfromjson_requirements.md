# Gateway Requirements

## Overview
This is the higher level API to create and manage a gateway via a JSON configuration file. Modules are added to the gateway, and communication "links" are established between modules via a message broker.

## References

### Parson
Third party C JSON library: https://github.com/kgabis/parson

## JSON Structure

```json
{
    "modules" :
    [ 
        {
            "module name" : "one",
            "loading args" : 
            {
                "module path" : "module1.dll"
            },
            "args" : ...
        },
        {
            "module name" : "two",
            "loading args" : 
            {
            "module path" : "module2.dll"
            },
            "args" : ...
        }
    ],
    "links":
    [
        {
            "source": "one",
            "sink": "two"
        }
    ]
}
```

## Exposed API
```
#ifdef __cplusplus
extern "C"
{
#endif

extern GATEWAY_HANDLE Gateway_CreateFromJson(const char* file_path);

#ifdef __cplusplus
}
#endif
```

##Gateway_CreateFromJson
```
extern GATEWAY_HANDLE Gateway_CreateFromJson(const char* file_path);
```
Gateway_CreateFromJson creates a new gateway using information contained within a well-formed JSON configuration file. The JSON string should be formatted as described above.

**SRS_GATEWAY_JSON_14_001: [** If `file_path` is NULL the function shall return NULL. **]**

**SRS_GATEWAY_JSON_14_002: [** The function shall use *parson* to read the file and parse the JSON string to a *parson* `JSON_Value` structure. **]**

**SRS_GATEWAY_JSON_14_003: [** The function shall return NULL if the file contents could not be read and/or parsed to a `JSON_Value`. **]**

**SRS_GATEWAY_JSON_14_004: [** The function shall traverse the `JSON_Value` object to initialize a `GATEWAY_PROPERTIES` instance. **]**

**SRS_GATEWAY_JSON_14_005: [** The function shall set the value of `const void* module_configuration` in the `GATEWAY_PROPERTIES` instance to a char\* representing the serialized *args* value for the particular module. **]**

**SRS_GATEWAY_JSON_17_005: [** The function shall parse the "loading args" for "module path" and fill a `DYNAMIC_LOADER_CONFIG` structure with the module path information. **]**

**SRS_GATEWAY_JSON_17_003: [** The function shall set the value of `const void * loader_configuration` in the `GATEWAY_PROPERTIES` instance to a pointer of the module's `DYNAMIC_LOADER_CONFIG` structure. **]**

**SRS_GATEWAY_JSON_14_006: [** The function shall return NULL if the `JSON_Value` contains incomplete information. **]**

**SRS_GATEWAY_JSON_04_001: [** The function shall create a Vector to Store all links to this gateway. **]**

**SRS_GATEWAY_JSON_04_002: [** The function shall add all modules source and sink to `GATEWAY_PROPERTIES` inside `gateway_links`. **]**

**SRS_GATEWAY_JSON_14_007: [** The function shall use the `GATEWAY_PROPERTIES` instance to create and return a `GATEWAY_HANDLE` using the lower level API. **]**

**SRS_GATEWAY_JSON_17_004: [** The function shall set the module loader to the default dynamically linked library module loader. **]** 

**SRS_GATEWAY_JSON_17_001: [** Upon successful creation, this function shall start the gateway. **]**

**SRS_GATEWAY_JSON_17_002: [** This function shall return `NULL` if starting the gateway fails. **]**

**SRS_GATEWAY_JSON_14_008: [** This function shall return `NULL` upon any memory allocation failure. **]**