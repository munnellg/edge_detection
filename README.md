# Edge Detection Demonstration

Simple tool for comparing the output of various edge
detectors. Included are:

+ Sobel
+ Roberts
+ Prewitt
+ Scharr
+ Sobel 5x5

Use the number keys to select an edge detector:

1. Sobel
2. Prewitt
3. Roberts
4. Scharr
5. Sobel 5x5

Press the 'o' key to display the original image. Press the 'e' key to
return the thresholded edge image.

Tool also normalizes gradient outputs so that they lie between 0
and 255. This is achieved through a couple of methods which the user
may switch between

+ Local Normalization (select by pressing 'l') normalizes purely with
  respect to the thresholded image. Max gradient is scaled up (or
  down) to 255. All other gradients are scaled accordingly
+ Global Normalization (select by pressing 'g') scales with respect to
  the maximum possible output of the Edge Detector's convolution
  matrix. Usually results in a much darker image, but can be used to
  show how the output gradients from Scharr might be much greater than
  those for Sobel or Prewitt
+ No Normalization (select by pressing 'n') turns off normalization
  completely. Will result in pixel values overflowing producing
  strange and generally meaningless output for all practical
  purposes. Still worth including

This code is for demonstration purposes only and should not be
included in any actual computer vison project.
