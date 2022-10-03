
<img src="http://rodolphe-vaillant.fr/images/2019-05/harmonic_function_triangle_mesh_banner.jpg" alt="2d spline">

Compute harmonic weights over a triangle mesh using the Laplacian matrix and cotangent weights.
The result is display with GLUT. <a href="rodolphe-vaillant.fr/index.php?e=20">For more details on the math see my article</a>

At the top of main.cpp you can play with hard coded parameters to change sample models or display mode:
- _3d_view
- _b_type
- _sample_path
- etc.
    
Toogle wireframe view with the 'w' key

The crux of the algorithm is in "solve_laplace_equation.cpp"

(MIT-license)

<link href="https://fonts.googleapis.com/css?family=Cookie" rel="stylesheet"><a class="bmc-button" target="_blank" href="https://www.buymeacoffee.com/jBnA3c2Fw"><img src="https://www.buymeacoffee.com/assets/img/BMC-btn-logo.svg" alt="Buy me a coffee"><span style="margin-left:5px">You can buy me a coffee o(^◇^)o</span></a> if you use my code in a commercial project or just want to support.
