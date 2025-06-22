# imagecutter

## Features
- Select areas on image
- Rotate selected areas
- Saving selected areas
- Fast GUI SDL3 interface, even for weak devices

## Installation

### Prerequisites
- SDL3
- SDL_Image

### Build Instructions
1. Clone the repository
    ```bash
    git clone https://github.com/rmntgx/imagecutter.git
    ```
2. Compile the program:
    ```bash
    make
    ```
### Controls
- Left click and drag to create selection
- Right click on selection to select it for rotation
- 'Q'/'E' keys to rotate selected area left/right
- 'S' key to save all selections
- 'C' key to clear all selections
- 'D' for delete selection
- 'N' key for next image
- 'P' key for previous image
- 'ESC' to quit
