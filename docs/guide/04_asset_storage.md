# Asset Storage

you can store media files in a folder in the dataset and address them in one of the tablets

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, file
> ```
> 
> ``` csv
> # event-file.csv
> visited Japan,IMG_0890.jpeg
> ```
> 
> ```
> # img/IMG_0890.jpeg
>                )\         O_._._._A_._._._O         /(
>                 \`--.___,'=================`.___,--'/
>                  \`--._.__                 __._,--'/
>                    \  ,. l`~~~~~~~~~~~~~~~'l ,.  /
>        __            \||(_)!_!_!_.-._!_!_!(_)||/            __
>        \\`-.__        ||_|____!!_|;|_!!____|_||        __,-'//
>         \\    `==---='-----------'='-----------`=---=='    //
>         | `--.                                         ,--' |
>          \  ,.`~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~',.  /
>            \||  ____,-------._,-------._,-------.____  ||/
>             ||\|___!`======="!`======="!`======="!___|/||
>             || |---||--------||-| | |-!!--------||---| ||
>   __O_____O_ll_lO_____O_____O|| |'|'| ||O_____O_____Ol_ll_O_____O__
>   o H o o H o o H o o H o o |-----------| o o H o o H o o H o o H o
>  ___H_____H_____H_____H____O =========== O____H_____H_____H_____H___
>                           /|=============|\
> ()______()______()______() '==== +-+ ====' ()______()______()______()
> ||{_}{_}||{_}{_}||{_}{_}/| ===== |_| ===== |\{_}{_}||{_}{_}||{_}{_}||
> ||      ||      ||     / |==== s(   )s ====| \     ||      ||      ||
> ======================()  =================  ()======================
> ----------------------/| ------------------- |\----------------------
>                      / |---------------------| \
> -'--'--'           ()  '---------------------'  ()
>                    /| ------------------------- |\    --'--'--'
>        --'--'     / |---------------------------| \    '--'
>                 ()  |___________________________|  ()           '--'-
>   --'-          /| _______________________________  |\
>  --' gpyy      / |__________________________________| \
> Art by Glory Moon
> ```

if you version control the dataset with git, you can add the asset folder to [Large File Storage](https://git-lfs.com/) 

```sh
git lfs install

git lfs track "img/**"

git add .gitattributes

git add img/IMG_0890.jpeg

git commit -m "add picture of Japan"
```

binary blobs SHOULD be stored in a folder inside the dataset directory, and associate each blob with a value of collection `filename`.
In datasets version controlled by `git`, the asset directory SHOULD be filtered with git-lfs or git-index.

To learn more about csvs, see other [User Guides](./README.md) and the [Requirements](../requirements.md).
