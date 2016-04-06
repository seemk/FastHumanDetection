## Fast human detection in depth images

An implementation of [Fast Human Detection for Indoor Mobile Robots Using Depth Images](http://www.cs.cmu.edu/~mmv/papers/13icra-CoBotPeopleDetection.pdf) for Kinect V2 depth images.

Differences from the paper:
* Kinect V2 over V1
* Region planarity checks are disabled by default
* A neural net is used for classification instead of SVM
* Stratified sampling is used over random sampling for point cloud construction

### Building

Standard CMake build.

Generate the Makefiles:
```
$ mdkr build
$ cd build
$ cmake ..
```

Or MSVC project files:
```
$ mdkr build
$ cd build
$ cmake -G "Visual Studio 12 Win64" -DCMAKE_PREFIX_PATH=KINECT_SDK_DIR ..
```
KINECT_SDK_DIR is usually `C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409`

Run `make` or build the MSVC projects.

### Training a dataset

fhd_ui can be used to create a training set from depth images.
```
$ ./fhd_ui training_out.db
```
"open database" selects a Sqlite DB of depth images. Clicking on a candidate (marking it green) sets it as a positive candidate (human).
Pressing space commits the candidates to the database, where selected (green) marks a human and unselected candidate marks a negative candidate.
pressing x advances to the next frame

After creating the training set, a classifier can be trained with `fhd_train`

```
$ ./fhd_train training_out.db classifier.nn
```

### TODO
* Provide pretrained classifier, datasets
