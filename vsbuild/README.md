# Converting Old VS Build #

## Visual Studio 2017 ##

* Double click on View3D32.dsp to launch Visual Studio

* Accept the various dialogs

* Change the SDK version to most recent in

```
Project->View3D32 Properties...->General->Windows SDK Version
```
* Enable function level linking (to avoid a conflict with `/ZI`)

```
Project->View3D32 Properties...->C/C++->Code Generation->Enable Function-Level Linking
```
