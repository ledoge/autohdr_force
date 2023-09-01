#include <windows.h>
#include <stdio.h>
#include <string.h>

#define D3D_KEY "SOFTWARE\\Microsoft\\Direct3D"

// Function to create the specified registry key
int CreateDirect3DRegistryKey() {
    HKEY hKey;
    LONG result;

    // Try to open the key first to check if it already exists
    result = RegOpenKeyExA(HKEY_CURRENT_USER, D3D_KEY, 0, KEY_READ, &hKey);

    if (result != ERROR_SUCCESS) {
        // If the key doesn't exist, create it
        result = RegCreateKeyExA(HKEY_CURRENT_USER, D3D_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

        if (result != ERROR_SUCCESS) {
            // Failed to create the key
            fprintf(stderr, "Error creating Direct3D key. Error code: %ld\n", result);
            return 1; // Return an error code
        } else {
            printf("Direct3D key created successfully.\n");
        }
    } else {
        // Key already exists
        printf("Found existing Direct3D key\n");
        RegCloseKey(hKey);
    }

    // Close the key
    RegCloseKey(hKey);

    return 0; // Return success
}

char *FindSubkeyByName(const char *name) {
    HKEY hKey;
    LONG result;

    // Try to open the key
    result = RegOpenKeyExA(HKEY_CURRENT_USER, D3D_KEY, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Error opening the registry key. Error code: %ld\n", result);
        return NULL;
    }

    // Enumerate subkeys
    DWORD index = 0;
    char subkeyName[MAX_PATH];
    DWORD subkeyNameSize = MAX_PATH;
    while (RegEnumKeyExA(hKey, index, subkeyName, &subkeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        // Check if the subkey name matches the provided name
        if (strcmp(subkeyName, "MostRecentApplication") != 0) {
            HKEY subKey;
            result = RegOpenKeyExA(hKey, subkeyName, 0, KEY_READ, &subKey);
            if (result == ERROR_SUCCESS) {
                char value[MAX_PATH];
                DWORD valueSize = sizeof(value);
                // Check if the subkey has a "Name" value
                result = RegQueryValueExA(subKey, "Name", NULL, NULL, (LPBYTE) value, &valueSize);
                if (result == ERROR_SUCCESS && strcmp(value, name) == 0) {
                    // Match found, return the full path to this key
                    RegCloseKey(subKey);
                    RegCloseKey(hKey);
                    char *fullPath = (char *) malloc(strlen(D3D_KEY) + strlen(subkeyName) + 2);
                    sprintf(fullPath, "%s\\%s", D3D_KEY, subkeyName);
                    return fullPath;
                }
                RegCloseKey(subKey);
            }
        }

        index++;
        subkeyNameSize = MAX_PATH;
    }

    RegCloseKey(hKey);
    return NULL; // No match found
}

int PrintD3DBehaviorsValue(const char *keyPath) {
    HKEY hKey;
    LONG result;

    // Try to open the key
    result = RegOpenKeyExA(HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Error opening the registry key. Error code: %ld\n", result);
        // this should never happen
        exit(1);
    }

    char d3dBehaviors[1024]; // Assuming the value won't exceed 1023 characters
    DWORD d3dBehaviorsSize = sizeof(d3dBehaviors);

    // Try to read the "D3DBehaviors" string value
    result = RegQueryValueExA(hKey, "D3DBehaviors", NULL, NULL, (LPBYTE) d3dBehaviors, &d3dBehaviorsSize);
    if (result == ERROR_SUCCESS) {
        printf("D3DBehaviors value: %s\n", d3dBehaviors);
        return 1;
    } else {
        printf("No existing D3DBehaviors value\n");
        return 0;
    }

    RegCloseKey(hKey);
}

char *FindFreeApplicationSubkey() {
    HKEY hKey;
    LONG result;
    int index = 0;
    char subkeyName[256]; // Assuming the subkey name won't exceed 255 characters

    // Try to open the key
    result = RegOpenKeyExA(HKEY_CURRENT_USER, D3D_KEY, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Error opening the registry key. Error code: %ld\n", result);
        return NULL;
    }

    // Iterate through subkeys to find the first available "free" ApplicationX
    while (1) {
        // Create the subkey name to check, e.g., "Application0", "Application1", ...
        snprintf(subkeyName, sizeof(subkeyName), "Application%d", index);

        // Try to open the subkey
        HKEY opened = NULL;
        result = RegOpenKeyExA(hKey, subkeyName, 0, KEY_READ, &opened);
        if (result != ERROR_SUCCESS) {
            // The subkey doesn't exist, so it's available
            RegCloseKey(opened);
            char *availableSubkey = (char *) malloc(strlen(subkeyName) + 1);
            strcpy(availableSubkey, subkeyName);
            return availableSubkey;
        }
        index++;
    }

    RegCloseKey(hKey);
    return NULL; // Shouldn't reach this point
}

int CreateRegistryEntry(const char *keyPath, const char *name) {
    HKEY hKey;
    LONG result;

    // Try to create/open the key
    result = RegCreateKeyExA(HKEY_CURRENT_USER, keyPath, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Error creating/opening the registry key. Error code: %ld\n", result);
        return 1; // Return an error code
    }

    // Write the 'Name' value
    result = RegSetValueExA(hKey, "Name", 0, REG_SZ, (const BYTE *) name, (DWORD) (strlen(name) + 1));
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Error writing the 'Name' value. Error code: %ld\n", result);
        RegCloseKey(hKey);
        return 1; // Return an error code
    }

    // Close the key
    RegCloseKey(hKey);
    return 0; // Return success
}

int getYesNoResponse(const char *query) {
    char userInput[256]; // Assuming a maximum input length of 255 characters
    int response;

    do {
        printf("%s (y/n): ", query);

        if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
            // Handle input error
            fprintf(stderr, "Error reading input.\n");
            return -1; // You can choose an appropriate error code here
        }

        // Remove trailing newline, if any
        userInput[strcspn(userInput, "\n")] = '\0';

        if (strcmp(userInput, "y") == 0) {
            response = 1;
        } else if (strcmp(userInput, "n") == 0) {
            response = 0;
        } else {
            printf("Invalid input. Please enter 'y' or 'n'.\n");
            response = -1; // Set an error value to indicate invalid input
        }
    } while (response == -1);

    return response;
}

BOOL SetRegistryValue(const char *subKeyPath, const char *value) {
    HKEY hKey;
    LONG result;

    // Open or create the registry key
    result = RegCreateKeyEx(HKEY_CURRENT_USER, subKeyPath, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Error opening or creating registry key: %ld\n", result);
        return FALSE;
    }

    if (value != NULL) {
        // Set the registry value if the provided string is not NULL
        result = RegSetValueEx(hKey, "D3DBehaviors", 0, REG_SZ, (BYTE *) value, strlen(value) + 1);
        if (result != ERROR_SUCCESS) {
            fprintf(stderr, "Error setting registry value: %ld\n", result);
            RegCloseKey(hKey);
            return FALSE;
        }
    } else {
        // Delete the registry value if the provided string is NULL
        result = RegDeleteValue(hKey, "D3DBehaviors");
        if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
            fprintf(stderr, "Error deleting registry value: %ld\n", result);
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    RegCloseKey(hKey);
    return TRUE;
}


int main(int argc, char *argv[]) {
    if (CreateDirect3DRegistryKey()) {
        return 1;
    }

    char path[PATH_MAX];

    if (argc == 2) {
        // A single command line argument provided, use it as the path
        strncpy(path, argv[1], PATH_MAX - 1);
    } else if (argc == 1) {
        // No command line argument provided, prompt the user for a path
        printf("Enter an exe name or full path: ");
        if (fgets(path, PATH_MAX, stdin) == NULL) {
            fprintf(stderr, "Error reading input.\n");
            return 1;
        }
        // Remove the newline character if present
        size_t len = strlen(path);
        if (len > 0 && path[len - 1] == '\n') {
            path[len - 1] = '\0';
        }
    } else {
        // More than one command line argument provided, print an error
        fprintf(stderr, "Error: Too many command line arguments.\n");
        return 1;
    }

    char *exe_name = path;
    // replace forward slashes with backslashes in path
    size_t len = strlen(path);
    for (size_t i = 0; i < len; i++) {
        if (path[i] == '/') {
            path[i] = '\\';
            exe_name = &path[i] + 1;
        }
    }

    char *key = FindSubkeyByName(path);

    int behaviorExists = 0;
    if (key) {
        printf("Found existing key for game (");
        {
            const char *lastPart = strrchr(key, '\\');

            if (lastPart != NULL) {
                printf("%s", lastPart + 1);
            } else {
                return 1;
            }
        }
        printf(")\n");
        behaviorExists = PrintD3DBehaviorsValue(key);
    } else {
        char *new_subkey = FindFreeApplicationSubkey();
        printf("No existing key found, will be created as %s\n", new_subkey);
        size_t keyLength = strlen(D3D_KEY "\\") + strlen(new_subkey) + 1; // +1 for null terminator

        // Allocate memory for the new key
        key = (char *) malloc(keyLength);

        // Copy the base key part
        strcpy(key, D3D_KEY "\\");

        // Concatenate the new subkey
        strcat(key, new_subkey);

        if (CreateRegistryEntry(key, path)) {
            return 1;
        }
    }

    int result = getYesNoResponse("Force enable Auto HDR?");
    if (result == 0) {
        if (behaviorExists) {
            result = getYesNoResponse("Delete existing D3DBehaviors?");
            if (result) {
                if (SetRegistryValue(key, NULL)) {
                    puts("Success!\n");
                }
            }
        } else {
            puts("Nothing to do");
        }
        return 0;
    }

    result = getYesNoResponse("Enable 10 bit?");

    char *behaviorVal;
    if (result) {
        behaviorVal = "BufferUpgradeOverride=1;BufferUpgradeEnable10Bit=1";
    } else {
        behaviorVal = "BufferUpgradeOverride=1";
    }

    if (SetRegistryValue(key, behaviorVal)) {
        puts("Success!\n");
    }

    return 0;

}
