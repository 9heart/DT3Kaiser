#!/bin/sh

rm cardsprocessed/*.png

for i in cards/*.png
do
    NP=`echo $i | sed s!cards!cardsprocessed!g`
    echo $NP

    convert $i mask.png -alpha Off -compose CopyOpacity -composite $NP
done

cd cardsprocessed

for i in *.png
do
    convert $i -resize 95% -background transparent -gravity center -extent 256x370 $i
    composite -compose Dst_Over ../border.png $i $i
    convert $i -filter Lanczos -resize 256x256\! $i
done

for i in *.png
do
    NP=`echo $i | sed s!png!mat!g`
    echo $NP

    cat ../template.mat | sed s!IMAGE!$i!g > $NP
done

montage -background none -geometry '256x256' *png atlas.png