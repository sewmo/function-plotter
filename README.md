# Description
A minimalistic 500-line function plotter built in C with the SDL2 framework and the TinyExpr library.

# Specifications
This project was built with the **Simple DirectMedia Layer** framework (SDL2).  
It utilizes the **TinyExpr** library for expression parsing, compiling and evaluating, link to the library here: [TinyExpr](https://github.com/codeplea/tinyexpr?tab=readme-ov-file).  
The user is able to provide an expression to plot, and the color utilized in **plotting**.  
**Zooming** is controlled via the mouse scrollwheel or via the arrow keys for zooming in or out. 
**Camera movement** is controlled via the **WASD** keys, allowing the user to navigate the visible domain of the function.
Holding click displays the cursor's domain-space **x** coordinate and the evaluated function value **f(x)** at that point.

# Dependencies
**SDL2**     - Simple DirectMedia Layer framework for graphics rendering.  
**SDL2_ttf** - For TTF font loading and rendering.
**TinyExpr** - For expression parsing, compiling and evaluating.  

# Controls
**Q:** Exit Program  
**Scroll Wheel:** Zoom In / Zoom Out  
**↑:** Zoom In (+10 snap)  
**↓:** Zoom Out (-10 snap)  
**W:** Move Up
**A:** Move Left
**S:** Move Down
**D:** Move Right

# Installation
Download the .ZIP file, extract it, and run the 'plot' executable, providing the necessary parameters.

# [Preview](preview.png)


