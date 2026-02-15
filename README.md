# Description
A minimalistic 300-line  math plotter built in C with the SDL2 framework and the TinyExpr library.

# Specifications
This project was built with the **Simple DirectMedia Layer** framework (SDL2).  
It utilizes the **TinyExpr** library for expression parsing, compiling and evaluating, link to the library here: [TinyExpr](https://github.com/codeplea/tinyexpr?tab=readme-ov-file).  
The user is able to provide an expression to plot, and the color utilized in plotting.  
Furthermore, zooming is controlled via the mouse scrollwheel, or via the arrow keys for zoom in or out to multiples of 10.  

# Dependencies
**SDL2**     - Simple DirectMedia Layer framework for graphics rendering.  
**SDL2_ttf** - TTF font loader and renderer.  
**TinyExpr** - Expression parsing, compiling and evaluating.  

# Controls
**Q:** Exit Program  
**Scroll Wheel:** Zoom In / Zoom Out  
**↑:** Zoom In (+10 snap)  
**↓:** Zoom Out (-10 snap)  

# Installation
Download the .ZIP file, extract it, and run the 'plot' executable, providing the necessary parameters.

# Image
<img width="803" height="827" alt="image" src="https://github.com/user-attachments/assets/9be9ab7a-2419-4428-9866-f8066925c08a" />

