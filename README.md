[![CircleCI](https://circleci.com/gh/facebookresearch/habitat-sim.svg?style=shield)](https://circleci.com/gh/facebookresearch/habitat-sim)
[![codecov](https://codecov.io/gh/facebookresearch/habitat-sim/branch/main/graph/badge.svg)](https://codecov.io/gh/facebookresearch/habitat-sim)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/facebookresearch/habitat-sim/blob/main/LICENSE)
[![Conda Version Badge](https://img.shields.io/conda/vn/aihabitat/habitat-sim?color=blue&label=conda%20version)](https://anaconda.org/aihabitat/habitat-sim)
[![Conda Platforms support Badge](https://img.shields.io/conda/pn/aihabitat/habitat-sim?color=orange&label=platforms)](https://anaconda.org/aihabitat/habitat-sim)
[![Documentation](https://img.shields.io/badge/docs-automated-green.svg)](https://aihabitat.org/docs/habitat-sim/)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)
[![Python 3.6, 3.7, 3.8](https://img.shields.io/badge/python-3.6%20%7C%203.7%20%7C%203.8-blue.svg)](https://www.python.org/downloads/release/)
[![Supports Bullet](https://img.shields.io/static/v1?label=supports&message=Bullet%20Physics&color=informational&link=https://opensource.google/projects/bullet3)](https://opensource.google/projects/bullet3)
[![Slack Join](http://img.shields.io/static/v1?label=Join%20us%20on&message=%23habitat-dev&labelColor=%234A154B&logo=slack)](https://join.slack.com/t/ai-habitat/shared_invite/enQtNjY1MzM1NDE4MTk2LTZhMzdmYWMwODZlNjg5MjZiZjExOTBjOTg5MmRiZTVhOWQyNzk0OTMyN2E1ZTEzZTNjMWM0MjBkN2VhMjQxMDI)
[![Twitter Follow](https://img.shields.io/twitter/follow/ai_habitat?style=social)](https://twitter.com/ai_habitat)

# Habitat-Sim

A high-performance physics-enabled 3D simulator with support for:
- 3D scans of indoor/outdoor spaces (with built-in support for [HM3D](https://aihabitat.org/datasets/hm3d/), [MatterPort3D](https://niessner.github.io/Matterport/), [Gibson](http://gibsonenv.stanford.edu/database/), [Replica](https://github.com/facebookresearch/Replica-Dataset), and other datasets)
- CAD models of spaces and piecewise-rigid objects (e.g. [ReplicaCAD](https://aihabitat.org/datasets/replica_cad/), [YCB](https://www.ycbbenchmarks.com/), [Google Scanned Objects](https://app.ignitionrobotics.org/GoogleResearch/fuel/collections/Google%20Scanned%20Objects)),
- Configurable sensors (RGB-D cameras, egomotion sensing)
- Robots described via URDF (mobile manipulators like [Fetch](http://docs.fetchrobotics.com/), fixed-base arms like [Franka](https://www.franka.de/),quadrupeds like [AlienGo](https://www.unitree.com/products/aliengo/)),
- Rigid-body mechanics (via [Bullet](https://github.com/bulletphysics/bullet3)).

The design philosophy of Habitat is to prioritize simulation speed over the breadth of simulation capabilities. When rendering a scene from the Matterport3D dataset, Habitat-Sim achieves several thousand frames per second (FPS) running single-threaded and reaches over 10,000 FPS multi-process on a single GPU. Habitat-Sim simulates a Fetch robot interacting in ReplicaCAD scenes at over 8,000 steps per second (SPS), where each ‘step’ involves rendering 1 RGBD observation (128×128 pixels) and rigid-body dynamics for 1/30sec.


Habitat-Sim is typically used with
[Habitat-Lab](https://github.com/facebookresearch/habitat-lab), a modular high-level library for end-to-end experiments in embodied AI -- defining embodied AI tasks (e.g. navigation, instruction following, question answering), training agents (via imitation or reinforcement learning, or no learning at all as in classical SensePlanAct pipelines), and benchmarking their performance on the defined tasks using standard metrics.

[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/facebookresearch/habitat-sim/)

[![Habitat Demo](https://img.shields.io/static/v1?label=WebGL&message=Try%20AI%20Habitat%20In%20Your%20Browser%20&color=blue&logo=webgl&labelColor=%23990000&style=for-the-badge&link=https://aihabitat.org/demo)](https://aihabitat.org/demo)

https://user-images.githubusercontent.com/2941091/126080914-36dc8045-01d4-4a68-8c2e-74d0bca1b9b8.mp4

---

## Table of contents
   1. [Citing Habitat](#citing-habitat)
   1. [Installation](#installation)
   1. [Testing](#testing)
   1. [Documentation](#documentation)
   1. [Datasets](#datasets)
   1. [External Contributions](#external-contributions)
   1. [License](#license)


## Citing Habitat
If you use the Habitat platform in your research, please cite the [Habitat](https://arxiv.org/abs/1904.01201) and [Habitat 2.0](https://arxiv.org/abs/2106.14405) papers:

```
@article{szot2021habitat,
  title     =     {Habitat 2.0: Training Home Assistants to Rearrange their Habitat},
  author    =     {Andrew Szot and Alex Clegg and Eric Undersander and Erik Wijmans and Yili Zhao and John Turner and Noah Maestre and Mustafa Mukadam and Devendra Chaplot and Oleksandr Maksymets and Aaron Gokaslan and Vladimir Vondrus and Sameer Dharur and Franziska Meier and Wojciech Galuba and Angel Chang and Zsolt Kira and Vladlen Koltun and Jitendra Malik and Manolis Savva and Dhruv Batra},
  journal   =     {arXiv preprint arXiv:2106.14405},
  year      =     {2021}
}

@inproceedings{habitat19iccv,
  title     =     {Habitat: {A} {P}latform for {E}mbodied {AI} {R}esearch},
  author    =     {Manolis Savva and Abhishek Kadian and Oleksandr Maksymets and Yili Zhao and Erik Wijmans and Bhavana Jain and Julian Straub and Jia Liu and Vladlen Koltun and Jitendra Malik and Devi Parikh and Dhruv Batra},
  booktitle =     {Proceedings of the IEEE/CVF International Conference on Computer Vision (ICCV)},
  year      =     {2019}
}
```

Habitat-Sim also builds on work contributed by others.  If you use contributed methods/models, please cite their works.  See the [External Contributions](#external-contributions) section
for a list of what was externally contributed and the corresponding work/citation.


## Installation

Habitat-Sim can be installed in 3 ways:
1. Via Conda - Recommended method for most users. Stable release and nightly builds.
1. [Experimental] Via PIP - `pip install .` to compile the latest headless build with Bullet. Read [build instructions and common build issues](BUILD_FROM_SOURCE.md).
1. Via Docker - Updated approximately once per year for [Habitat Challenge](https://aihabitat.org/challenge/).  Read [habitat-docker-setup](https://github.com/facebookresearch/habitat-lab#docker-setup).
1. Via Source - For active development. Read [build instructions and common build issues](BUILD_FROM_SOURCE.md).

### [Recommended] Conda Packages

Habitat is under active development, and we advise users to restrict themselves to [stable releases](https://github.com/facebookresearch/habitat-sim/releases). Starting with v0.1.4, we provide [conda packages for each release](https://anaconda.org/aihabitat).

#### Preparing conda env
Assuming you have [conda](https://docs.conda.io/projects/conda/en/latest/user-guide/install/) installed, let's prepare a conda env:
```bash
# We require python>=3.6 and cmake>=3.10
conda create -n habitat python=3.6 cmake=3.14.0
conda activate habitat
```

#### conda install habitat-sim
Pick one of the options below depending on your system/needs:

- To install on machines with an attached display:
   ```bash
     conda install habitat-sim -c conda-forge -c aihabitat
   ```
- To install on headless machines (i.e. without an attached display, e.g. in a cluster) and machines with multiple GPUs:
   ```
   conda install habitat-sim headless -c conda-forge -c aihabitat
   ```
- [**Most common scenario**] To install habitat-sim with bullet physics
   ```
   conda install habitat-sim withbullet -c conda-forge -c aihabitat
   ```

- Note: Build parameters can be chained together. For instance, to install habitat-sim with physics on headless machines:
   ```
   conda install habitat-sim withbullet headless -c conda-forge -c aihabitat
   ```

Conda packages for older versions can installed by explicitly specifying the version, e.g. `conda install habitat-sim=0.1.6 -c conda-forge -c aihabitat`.

We also provide a [nightly conda build for the main branch](https://anaconda.org/aihabitat-nightly). However, this should only be used if you need a specific feature not yet in the latest release version. To get the nightly build of the latest main, simply swap `-c aihabitat` for `-c aihabitat-nightly`.

## Testing

1. Let's download some 3D assets using our python data download utility:
   - Download (testing) 3D scenes
      ```bash
      python -m habitat_sim.utils.datasets_download --uids habitat_test_scenes --data-path /path/to/data/
      ```
      Note that these testing scenes do not provide semantic annotations.
      If you would like to test the semantic sensors via `example.py`, please use the data from the Matterport3D dataset (see [Datasets](DATASETS.md)).

   - Download example objects
      ```bash
      python -m habitat_sim.utils.datasets_download --uids habitat_example_objects --data-path /path/to/data/
      ```

1. **Interactive testing**: Use the interactive viewer included with Habitat-Sim
   ```bash
   # ./build/viewer if compiling locally
   habitat-viewer /path/to/data/scene_datasets/habitat-test-scenes/skokloster-castle.glb
   ```
   You should be able to control an agent in this test scene.
   Use W/A/S/D keys to move forward/left/backward/right and arrow keys or mouse to control gaze direction (look up/down/left/right).
   Try to find the picture of a woman surrounded by a wreath.
   Have fun!

1. **Physical interactions**: Use the interactive viewer with physics enabled:
   ```bash
   # ./build/viewer if compiling locally
   habitat-viewer --enable-physics --object-dir data/objects/example_objects -- data/scene_datasets/habitat-test-scenes/apartment_1.glb
   ```

   You should be able to insert objects into the scene by pressing 'o'. The viewer application outputs the full list of keyboard shortcuts to the console at runtime.

1. **Non-interactive testing**: Run the example script:
   ```bash
   python /path/to/habitat-sim/examples/example.py --scene /path/to/data/scene_datasets/habitat-test-scenes/skokloster-castle.glb
   ```
   The agent will traverse a particular path and you should see the performance stats at the very end, something like this:
  `640 x 480, total time: 3.208 sec. FPS: 311.7`.

   To reproduce the benchmark table from [Habitat ICCV'19](https://arxiv.org/abs/1904.01201) run `examples/benchmark.py --scene /path/to/mp3d_example/17DRP5sb8fy/17DRP5sb8fy.glb`.

   Additional arguments to `example.py` are provided to change the sensor configuration, print statistics of the semantic annotations in a scene, compute action-space shortest path trajectories, and set other useful functionality. Refer to the `example.py` and `demo_runner.py` source files for an overview.

   Load a specific MP3D or Gibson house: `examples/example.py --scene path/to/mp3d/house_id.glb`.


   We have also provided an [example demo](https://aihabitat.org/docs/habitat-lab/habitat-lab-demo.html) for reference.

   To run a physics example in python (after building with "Physics simulation via Bullet"):
   ```bash
   python examples/example.py --scene /path/to/data/scene_datasets/habitat-test-scenes/skokloster-castle.glb --enable_physics
   ```
   Note that in this mode the agent will be frozen and oriented toward the spawned physical objects. Additionally, `--save_png` can be used to output agent visual observation frames of the physical scene to the current directory.


### Common testing issues

- If you are running on a remote machine and experience display errors when initializing the simulator, e.g.
   ```bash
    X11: The DISPLAY environment variable is missing
    Could not initialize GLFW
   ```

  ensure you do not have `DISPLAY` defined in your environment (run `unset DISPLAY` to undefine the variable)

- If you see libGL errors like:

   ```bash
    X11: The DISPLAY environment variable is missing
    Could not initialize GLFW
   ```

    chances are your libGL is located at a non-standard location. See e.g. [this issue](https://askubuntu.com/questions/541343/problems-with-libgl-fbconfigs-swrast-through-each-update).

## Documentation

Browse the online [Habitat-Sim documentation](https://aihabitat.org/docs/habitat-sim/index.html).

To get you started, see the [Lighting Setup tutorial](https://aihabitat.org/docs/habitat-sim/lighting-setups.html) for adding new objects to existing scenes and relighting the scene & objects. The [Image Extractor tutorial](https://aihabitat.org/docs/habitat-sim/image-extractor.html) shows how to get images from scenes loaded in Habitat-Sim.

### Questions?
[![Slack Join](http://img.shields.io/static/v1?label=Join%20us%20on&message=%23habitat-dev&labelColor=%234A154B&logo=slack)](https://join.slack.com/t/ai-habitat/shared_invite/enQtNjY1MzM1NDE4MTk2LTZhMzdmYWMwODZlNjg5MjZiZjExOTBjOTg5MmRiZTVhOWQyNzk0OTMyN2E1ZTEzZTNjMWM0MjBkN2VhMjQxMDI)

## Datasets

[Common datasets used with Habitat](DATASETS.md).


## External Contributions

* If you use the noise model from PyRobot, please cite the their [technical report](https://github.com/facebookresearch/pyrobot#citation).


    Specifically, the noise model used for the noisy control functions named `pyrobot_*` and defined in `habitat_sim/agent/controls/pyrobot_noisy_controls.py`


* If you use the Redwood Depth Noise Model, please cite their [paper](http://redwood-data.org/indoor/)

    Specifically, the noise model defined in `habitat_sim/sensors/noise_models/redwood_depth_noise_model.py` and `src/esp/sensor/RedwoodNoiseModel.*`


## License

Habitat-Sim is MIT licensed. See the [LICENSE](LICENSE) for details.
