#include "mr_protocol.h"
marshall &operator<<(marshall &m, const submitmapresponse1 &s) {
    // Lab3: Your code here
     m << s.index;
    return m;
}

unmarshall &operator>>(unmarshall &u, submitmapresponse1 &s) {
    // Lab3: Your code here
    u >> s.index;
    return u;
}