rm ph*
particle compile photon
particle flash --usb $(ls -t ph* | head -n 1)
#particle flash Neo $(ls -t ph* | head -n 1)
