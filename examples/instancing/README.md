![example](example.gif)

The above demo has over 40000 textures being rotated and moved each frame.

This is one of the more advanced demos to really push performance, but in its simplest
form all you need to perform instancing is a list of `VK2DDrawInstace` and a single draw
call. It makes use of multiple worker threads primarily to divvy up the work involved in
constructing tens of thousands of model matrices, then all of that data is submitted in a
single draw call.

By default, this example sets the vram page size to 4mb and then creates two `VK2DDrawInstace`
lists of as many as the renderer can handle minus a few for the cameras - this happens to
be about 40000. A basic overview is

 1. Two `VK2DDrawInstace` lists of equal size are created so that they may be double-buffered
 2. A list of `Entity` is created of equal size to the instance lists, and its the main list that is updated
 3. One extra thread is created per CPU core and that thread is assigned a specific portion of the instance/entity list to update
 4. The worker threads process their portion of the list, and the main thread waits for them to finish and swaps the
 buffers so the worker threads can begin work on the next frame while the render thread work on rendering the frame is just got