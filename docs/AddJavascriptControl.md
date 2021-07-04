Add Javascript Control
===
To add control of a new C++ variable or c++ class to javascript, read below.



Add an Entire Sub Object
===
Often when dealing with other code or modules, include a new .c/.h which generate their own class types.  Often (in RadioEstimate or EventDsp) we will instantiate a single instance of this new object, and we wish to control it from javascript.

Example: Adding a pid filter the Flat way
===
* Flat means we dont make a new namespace under sjs.
* Each variable should have a prefix to avoid name conflicts
  * This is a major drawback as it pollutes the RadioEstimate namespace

Code
---

If the RadioEstimate.hpp has a new line:
```c
PID *pid;
```

* You need to make all the members public:
* You need to add some of the following:
```c
_NAPI_MEMBER(rpath pid-pimpl->,a, bool); \
_NAPI_MEMBER(rpath pid-pimpl->,b, bool); \
_NAPI_MEMBER(rpath pid-pimpl->,c, bool); \
```

and below:
```c
MEMB_LINE(ridx,a); \
MEMB_LINE(ridx,b); \
MEMB_LINE(ridx,c); \
```

Check you can access them with:

```javascript
sjs.attached.a
sjs.attached.b
sjs.attached.c += 4
````


Example: Half Nested
===
* This approach still suffers the drawback of polluting RadioEstimate namespace
* Benefit is that you can still have javascript access in the nice namespace
* This might not work for RE

Add this to `wrap_native.js`
```javascript
```




Example: Adding a pid filter the nested way
===
Note this is TWICE as hard for RadioEstimate due to the template torture going on.
This would be much eaiser from eventDsp as there is only 1 instance of that.
