
mkdir assets\language
mkdir assets\font
mkdir assets\skin
mkdir assets\demo_skin
copy /Y "..\tinkerbell\*.txt" "assets\language\*"
copy /Y "..\tinkerbell\default_font\segoe_white_with_shadow*" "assets\font\*"
copy /Y "..\tinkerbell\default_font\orangutang*" "assets\font\*"
copy /Y "..\tinkerbell\default_skin\*.png" "assets\skin\*"
copy /Y "..\tinkerbell\default_skin\*.txt" "assets\skin\*"
copy /Y "..\Demo\demo01\skin\*.png" "assets\demo_skin\*"
copy /Y "..\Demo\demo01\skin\*.txt" "assets\demo_skin\*"