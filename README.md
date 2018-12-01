# wol-remapper
Remap one wake on lan address to one (or more) others

## Usage
Start on the command line with no arguments, all mac addresses are hardcoded in `in_mac` and `to_wake`. Magic packets that are picked up are filtered by `in_mac`.

## Security
Make sure this is run on a closed network, where you filter broadcast packets. This application has not been thouroughy tested for immunity to DDOS attacks, or any other kind of vulnerability.
