This fork adds DG-ID feature to YSF processing.
This fork solves problem with YSF photo packets comming from HandHeld to Network that was lost in original MMDVMHost version. It is because modem can't get Headers so fast, so modem lost headers of data packets. We regenerate lost headers packets so we can get photo data packets back.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.
