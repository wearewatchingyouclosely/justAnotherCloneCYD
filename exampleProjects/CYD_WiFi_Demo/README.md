# CYD WiFi Demo - Credential Setup

This demo uses a separate `.tkn` file to store WiFi credentials, keeping them out of version control.

## Setup Instructions

1. **Create the credentials file:**
   
   Create a file named `wifi_credentials.tkn` in this folder (`CYD_WiFi_Demo/`).

2. **Add your credentials:**
   
   Paste the following into `wifi_credentials.tkn`, replacing with your actual WiFi details:

   ```cpp
   // WiFi Credentials - DO NOT COMMIT TO GIT
   #define WIFI_SSID     "YourNetworkName"
   #define WIFI_PASSWORD "YourPassword"
   ```

3. **Compile and upload** the sketch as normal.

## Why .tkn files?

- The `.tkn` extension is added to `.gitignore` so credentials won't be accidentally committed
- The sketch will fail to compile if the file is missing, reminding you to create it
- Easy to have different credentials on different machines

## Troubleshooting

**Compiler error: "wifi_credentials.tkn: No such file or directory"**
- You need to create the `wifi_credentials.tkn` file as described above

**WiFi won't connect**
- Double-check your SSID spelling (case-sensitive)
- Verify your password is correct
- Ensure your network is 2.4GHz (ESP32 doesn't support 5GHz)
