// SPDX-License-Identifier: GPL-3.0-or-later
#include <app/Activity.h>

// The LED matrix is driven independently of the invisible EasyUI canvas.
// main.ftu is the unmodified empty activity asset from the pinned Ulanzi demo.
class mainActivity : public Activity {
protected:
    const char* getAppName() const override { return "main.ftu"; }
};
REGISTER_ACTIVITY(mainActivity);
