# ESP-CAM-Robot

This repository contains the PlatformIO project for the **ESP32-CAM Development Board** used in the **Hazard Detection Robot** project.

## How to Use

1. **Clone this repository**:
    ```bash
    git clone https://github.com/jithangowda/ESP-CAM-Robot.git
    ```

2. **Download and Install Visual Studio Code**:
    - Go to [VSCode Downloads](https://code.visualstudio.com/Download) and install it.

3. **Install the PlatformIO Extension** inside Visual Studio Code:
    - Open VSCode, go to Extensions (`Ctrl+Shift+X`).
    - Search for **PlatformIO IDE** and click Install.

4. **Open the cloned project**:
    - After installing PlatformIO, open VSCode and click the **PlatformIO** icon in the left sidebar.
    - Click **Open Project** and select the folder where you cloned this repo.

5. **Create a `secrets.h` file**:
    - Inside the `include/` directory, create a new file named `secrets.h`.
    - Add the following content:
    ```cpp
    #ifndef SECRETS_H
    #define SECRETS_H

    // Primary Wi-Fi Credentials
    const char *ssid1 = "***********"; 
    const char *password1 = "***********";

    // Secondary Wi-Fi Credentials
    const char *ssid2 = "***********";
    const char *password2 = "***********";

    #endif
    ```
    - Fill in your primary Wi-Fi SSID and password.
    - If you don't have a secondary Wi-Fi network, you can leave it as it is.

6. **Build and Upload** using PlatformIO interface:
    - In the PlatformIO toolbar (at the bottom), click **Build** to compile the code.
    - Once the build is successful, click **Upload** to flash the code to your ESP32-CAM.

## Requirements

- ESP32-CAM Development Board
- Visual Studio Code
- PlatformIO Extension
