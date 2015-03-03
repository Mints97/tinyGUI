tinyGUI
=======

A small and easy-to-use object-oriented Windows GUI library for C

Currently at version 0.3 (Alpha)

tinyGUI is a (tiny) object-oriented Windows GUI library for C. It was designed with ease of use being the primary concern.
tinyGUI sports a minimalist design, which contributes to its small size. However, all problems caused by abscence of certain functionality
from this or other versions is mitigated by easy extendibility.

To use tinyGUI, just add tinyGUI.h and tinyGUI.c to your project, 
`#include "tinyGUI/tinyGUI.h"`
and start coding!

tinyGUI is currently a work-in-progress, it is still missing many widgets and functionality that should make work easier.
A demonstration of some of its current capabilities can be found in demo.c (a small educational application for children) and in the 2048 folder (a minimalist implementation of the infamous 2048 game). Stay tuned!

P.S. This is the first project I've ever hosted at GitHub, sorry if I've messed up somehow!

#Object - oriented system

tinyGUI uses a custom object-oriented system called tinyObject, designed to create objects that are as easy to use 
as to extend.

Check out [tinyObject's GitHub page](https://github.com/Mints97/tinyObject) for its syntaxis.





#Library structure

tinyGUI has, so far, the following class inheritance hierarchy:

- Object 
  - EventArgs
    - MouseEventArgs
  - Pen
  - Brush
  - GUIObject
    - Window
    - Control
      - Button
      - TextBox
      - Label

A description of every class follows.


#Class Object

Inheritance: base class

The main base class in tinyGUI, all the other classes directly or indirectly inherit from it.

###Fields

```C
enum _objectType type; /* Identifies the object type */
CRITICAL_SECTION criticalSection; /* The critical section for synchronising access to the object */
```

*Note: the second parameter should be initialized as a critical section (by calling InitializeCriticalSection(object->criticalSection)), which is done automatically in its constructor and in the constructors of all its derived classes. 
It can then be used anywhere in the code to synchronize access to an object. You can use the following utility macros for this:*
```C
startSync(object)
endSync(object)
```

###Constructors

These are all default.


#Class EventArgs

Inheritance: base class, inherits from Object

Instances of this type are passed to event handler callbacks. They contain information about the Windows message
that triggered the event, along with the additional Windows parameters.

###Fields

```C
UINT message; /* The Windows message */
WPARAM wParam; /* The first Windows message additional parameter */
LPARAM lParam; /* The second Windows message additional parameter */
```

###Methods

```C
/* Virtual method. Updates the field values of the object corresponding to the parameters. Actually points to the class initializer
of it type */
void updateValue(UINT message, WPARAM wParam, LPARAM lParam);
```

###Constructors

```C
/* Both constructors set the field values of the object corresponding to their parameters */
void initEventArgs(EventArgs thisObject, UINT message, WPARAM wParam, LPARAM lParam);
EventArgs newEventArgs(UINT message, WPARAM wParam, LPARAM lParam);
```


#Class MouseEventArgs

Inheritance: inherits from EventArgs

Instances of this type are passed to event handler callbacks when a mouse message is recieved.

###Fields

```C
int cursorX; /* The X position of the cursor at the time the message was sent */
int cursorY; /* The Y position of the cursor at the time the message was sent */
```

###Constructors

```C
/* Both constructors set the field values of the object corresponding to their parameters */
void initMouseEventArgs(MouseEventArgs thisObject, UINT message, WPARAM wParam, LPARAM lParam);
MouseEventArgs newMouseEventArgs(UINT message, WPARAM wParam, LPARAM lParam);
```


#Class GUIObject

Inheritance: base class, inherits from Object

This is the main widget type; all the widgets directly or indirectly inherit from it.

###Fields

```C
LONG_PTR origProcPtr; /* The pointer to the original window procedure */ 
	
HWND handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */ 
HINSTANCE moduleInstance; /* The current module instance */ 
PAINTSTRUCT paintData; /* A structure with data about the GUIObject's painting */
HDC paintContext; /* A handle to the window's current paint context. Initialized internally on receiving a WM_PAINT message */
HDC offscreenPaintContext; /* A handle to the window's offscreen paint context. Used in double-buffering drawing optimizations */
HBITMAP offscreenBitmap; /* A handle to the window's offscreen bitmap instance. Used in double-buffering drawing optimizations */
BOOL customEraseBG; /* If this is TRUE, default processing for the WM_ERASEBKGND doesn't occur so custom processing in an event handler
    can be used. Useful for preventing flickering. */

char *className; /* The name of the window/control's WinAPI "class" */ 
HMENU ID; /* The child-window/control identifier */ 
DWORD styles; /* The window/control styles (WinAPI predefined macro values) */ 
DWORD exStyles; /* The window/control extended styles (WinAPI predefined macro values) */ 

struct _event *events; /* An array of the GUIObject's events */
unsigned int numEvents; /* The number of events registered for the GUIObject */

char *text; /* The window/control text */ 

int width; /* The window/control width, pixels */
int height; /* The window/control height, pixels */

int minWidth; /* The window/control minimum width, pixels */
int minHeight; /* The window/control minimum height, pixels */

int maxWidth; /* The window/control maximum width, pixels */
int maxHeight; /* The window/control maximum height, pixels */

int realWidth; /* The window/control width used in anchor calculations. Not affected by min and max settings */
int realHeight; /* The window/control height used in anchor calculations. Not affected by min and max settings */

int x; /* The window/control x position (from left), pixels */
int y; /* The window/control y position (from top), pixels */

int realX; /*  used in anchor calculations. Not affected by min and max settings */
int realY; /*  used in anchor calculations. Not affected by min and max settings */

BOOL enabled; /* The GUIObject's enabled state */
```

###Methods

*Note: From here on, all BOOL methods return TRUE on success and FALSE on failure. I plan to replace this in the 
future with an error-handling mechanism.*

```C
/* Adds a child object to a GUIObject. Child objects are displayed along with their parent */
BOOL addChild(GUIObject child);

/* Removes a child object from a GUIObject */
BOOL removeChild(GUIObject child);

/* Sets an event for a GUIObject by a Windows message. This method returns a unique event identifier for the object
   which can be later used in other event functions.
   The message parameter is a Windows message recieved through the window proc. It can also be a WM_COMMAND type message, like BN_CLICKED.
   Standard window messages apply for all controls, as they are subclassed by default.
   The callback parameter is a pointer to the callback event handler function. It should have the following declaration:
   void callback(GUIObject sender, void *context, EventArgs e)
   The context parameter is a pointer that is passed to the callback on every invocation. This is done to handle events
   in a thread-safe way.
   The mode parameter is the synchronization mode, it should be SYNC for synchronous events and ASYNC for asynchronous events. */
int setEvent(DWORD message, void(*callback)(GUIObject, void*, EventArgs),
					 void *context, enum _syncMode mode);
	/* Note: if the sender of an event is a type derived from GUIObject, or the event's arguments are of a type derived from EventArgs,
	  it is preferred to cast them inside the function, along with the context parameter. However, you can also declare the callback with
	  the actual expected types, like this, for example:
	  void onClick(Button sender, Window mainWindow, MouseEventArgs e)
	  and cast its pointer to the appropriate callback type (you can use the Callback typedef):
	  $(button)->setOnClick((Callback)onClick, (void*)mainWindow, SYNC);
	  Beware! This may be easier for you to use, but this technically causes undefined behaviour, according to the C standard, as this means
	  that tinyGUI will call a function pointer typecasted from an incompatible function pointer type. However, in practice, in most modern 
	  compilers on most modern systems, this is nearly always safe. */

/* Sets a condition for an event of a GUIObject. The condition parameter is a pointer to a BOOL variable, the value of which should
   serve as a condition for the event handling referred to by eventID. I.e. if the value of this variable is TRUE, the event handling 
   occurs, if it is FALSE, the handling doesn't occur */
BOOL setEventCondition(int eventID, BOOL *condition);

/* Sets an interrupt state for an event of a GUIObject. If the interrupt parameter is TRUE, the default processing for the message
   referred to by eventID doesn't occur. Otherwise, it occurs normally. */
BOOL setEventInterrupt(int eventID, BOOL interrupt);

/* Changes an event's enabled state for a GUIObject. If enabled is set to FALSE, the event handling referred to by eventID doesn't happen.
   It happens normally otherwise. */
BOOL setEventEnabled(int eventID, BOOL enabled);

/* Sets a WM_LBUTTONUP event for an (enabled) GUIObject */
int setOnClick(void(*callback)(GUIObject, void*, EventArgs), void *context, enum _syncMode mode);

/* Virtual method. Moves a GUIObject to a new location specified by x and y */
BOOL setPos(int x, int y);

/* Resizes a GUIObject to a new size specified by width and height */
BOOL setSize(int width, int height);

/* Sets a GUIObject's minimum size to a new value specified by minWidth and minHeight */
BOOL setMinSize(int minWidth, int minHeight);

/* Sets a GUIObject's maximum size to a new value specified by maxWidth and maxHeight */
BOOL setMaxSize(int maxWidth, int maxHeight);

/* Sets a new text for a GUIObject */
BOOL setText(char *text);

/* Sets a GUIObject's enabled state */
BOOL setEnabled(BOOL enabled);

/* Draws a line in a GUIObject from the point specified by x1, y1 to the point specified by x2, y2 with a pen specified by the 
   parameter pen. If pen is NULL, a null pen is used. Drawing is double-buffered */
BOOL drawLine(Pen pen, int x1, int y1, int x2, int y2);

/* Draws an arc in a GUIObject inside the bounds of a rectangle specified by boundX1, boundY1, boundX2, and boundY2
   (the upper left and the bottom right corners of the rectangle, respectively), from a point specified by x1 and y1 to
   the point specified by x2, y2 with a pen specified by the parameter pen. If pen is NULL, a null pen is used. 
   Drawing is double-buffered */
BOOL drawArc(Pen pen, int boundX1, int boundY1, int boundX2, int boundY2, int x1, int y1, int x2, int y2);

/* Draws a rectangle in a GUIObject specified by boundX1, boundY1, boundX2, and boundY2 (the upper left and the bottom right corners of the 
   rectangle, respectively) with a pen specified by the parameter pen and fills it with the brush specified by the parameter brush. If pen is NULL, a null pen is used. If brush is NULL, a hollow brush is used. Drawing is double-buffered */
BOOL drawRect(Pen pen, Brush brush, int boundX1, int boundY1, int boundX2, int boundY2);

/* Draws a rounded rectangle in a GUIObject specified by boundX1, boundY1, boundX2, and boundY2 (the upper left and the bottom right 
   corners of the rectangle, respectively) with an ellipse of width ellipseWidth and height ellipseHeight being used to draw the 
   rounded edges, with a pen specified by the parameter pen, and fills it with the brush specified by the parameter brush. If pen is NULL, a null pen is used. If brush is NULL, a hollow brush is used. Drawing is double-buffered */
BOOL drawRoundedRect(Pen pen, Brush brush, int boundX1, int boundY1, 
							int boundX2, int boundY2, int ellipseWidth, int ellipseHeight);

/* Draws an ellipse in a GUIObject inside the bounds of a rectangle specified by boundX1, boundY1, boundX2, and boundY2 
   (the upper left and the bottom right corners of the rectangle, respectively) with a pen specified by the parameter pen 
   and fills it with the brush specified by the parameter brush. If pen is NULL, a null pen is used. If brush is NULL, 
   a hollow brush is used. Drawing is double-buffered */
BOOL drawEllipse(Pen pen, Brush brush, int boundX1, int boundY1, int boundX2, int boundY2);

/* Draws a polygon in a GUIObject between a set of points (number given by numPoints) specified by the coords array, which should 
   contain the points in the format {x1, y1, x2, y2, x3, y3, ... xn, yn}, with a pen specified by the parameter pen 
   and fills it with the brush specified by the parameter brush. If pen is NULL, a null pen is used. If brush is NULL, 
   a hollow brush is used. Drawing is double-buffered */
BOOL drawPolygon(Pen pen, Brush brush, int numPoints, LONG *coords);
```

###Constructors

```C
/* The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the GUIObject's text, width and height specify its initial size */
void initGUIObject(GUIObject thisObject, HINSTANCE instance, char *text, int width, int height);
GUIObject newGUIObject(HINSTANCE instance, char *text, int width, int height);
```



#Class Window

Inheritance: inherits from GUIObject

This class represents a window.

###Fields

```C
int clientWidth; /* The client area's width, in pixels */
int clientHeight; /* The client area's height, in pixels */

BOOL resizable; /* The window's resizable state */
BOOL maximizeEnabled; /* The window's maximize box state */
```

###Methods

```C
/* Changes a window's resizable style */
BOOL setResizable(BOOL resizable);

/* Enables or disables a window's maximize box */
BOOL enableMaximize(BOOL maximize);
```

###Constructors

```C
/* The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the window's text, width and height specify its initial size */
void initWindow(Window thisObject, HINSTANCE instance, char *text, int width, int height);
Window newWindow(HINSTANCE instance, char *text, int width, int height);
```


#Class Control

Inheritance: base class, inherits from GUIObject

This is the base class for all controls.

###Fields

```C
int minX; /* The minimum x position of the control (distance from parent's left edge), pixels */
int minY; /* The minimum y position of the control (distance from parent's top edge), pixels */

int maxX; /* The maximum x position of the control (distance from parent's left edge), pixels */
int maxY; /* The maximum y position of the control (distance from parent's top edge), pixels */

short anchor; /* The anchor settings for the control. It can be a bitwise addition (OR) of the following values:
				  ANCHOR_LEFT (0xF000), ANCHOR_RIGHT (0x000F), ANCHOR_TOP (0x0F00), ANCHOR_BOTTOM (0x00F0).
				  If an anchor is used, the control is moved or resized so that its borders remain at a constant
				  distance from the respective edges of its parent (limited by minWidth, minHeight, maxWidth, maxHeight,
				  minX, minY, maxX and maxY settings for the control). If neither of the two anchors for an orientation
				  (horizontal or vertical) are specified, the control is anchored to the center of its parent in this orientation. */
```

###Methods

```C
/* Sets a control's minimum position to a new value specified by minX and minY */
BOOL setMinPos(int minX, int minY);

/* Sets a control's maximum position to a new value specified by maxX and maxY */
BOOL setMaxPos(int maxX, int maxY);
```

###Constructors

```C
/* The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the control's text, width and height specify its initial size,
   x and y specify its initial position */
void initControl(Control thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height);
Control newControl(HINSTANCE instance, char *text, int x, int y, int width, int height);
```



#Class Button

Inheritance: inherits from Control

This class represents a button.

###Constructors

```C
/* The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the button's text, width and height specify its initial size,
   x and y specify its initial position */
void initButton(Button thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height);
Button newButton(HINSTANCE instance, char *text, int x, int y, int width, int height);
```

#Class TextBox

Inheritance: inherits from Control

This class represents a textbox (text input field).

###Fields

```C
BOOL multiline; /* The textbox's multiline style */
BOOL numOnly; /* The textbox's number only style - if it accepts only numbers or not */
```

###Methods

```C
/* Sets the text input mode for a textbox to number-only or to not number-only */
BOOL setNumOnly(BOOL numOnly);
```

###Constructors

```C
/* The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the textbox's text, width and height specify its initial size,
   x and y specify its initial position. The parameter multiline specifies the textbox's multiline style and can have the values
   MULTILINE and SINGLELINE */
void initTextBox(TextBox thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height, enum _textboxtype multiline);
TextBox newTextBox(HINSTANCE instance, char *text, int x, int y, int width, int height, enum _textboxtype multiline);
```

#Class Label

Inheritance: inherits from Control

This class represents a label (static text field).

###Constructors

```C
/* The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the label's text, width and height specify its initial size,
   x and y specify its initial position */
void initLabel(Label thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height);
Label newLabel(HINSTANCE instance, char *text, int x, int y, int width, int height);
```


#Class Pen

Inheritance: inherits from Object

This class represents a logical pen that can be used to draw lines.

###Fields

```C
HPEN handle; /* The pen's handle */
int penStyle; /* The pen's style (WinAPI predefined macro values) */
int width; /* The pen's width/thickness */
COLORREF *color; /* The pen's color in RGB (can be defined with the RGB(r, g, b) WinAPI macro) */
```

###Constructors

```C
/* Sets the field values to the values of the respective parameters */
void initPen(Pen thisObject, int penStyle, int width, COLORREF color);
Pen newPen(int penStyle, int width, COLORREF color);
	/* Note: these constructors are deprecated as they basically evaluate to a call to WinAPI function CreatePen with the same parameters. They are to be replaced with more useful methods in the next versions. */
```


#Class Brush

Inheritance: inherits from Object

This class represents a logical brush that can be used to fill areas.

###Fields

```C
HBRUSH *handle; /* The brush's handle */
UINT *brushStyle; /* The brush's style (WinAPI predefined macro values) */
COLORREF *color; /* The brush's color in RGB (can be defined with the RGB(r, g, b) WinAPI macro) */
ULONG_PTR *hatch; /* Either a predefined macro value of a WinAPI hatch style, or a handle to a bitmap with a pattern */
```

###Constructors

```C
/* Sets the field values to the values of the respective parameters */
void initBrush(Brush thisObject, UINT brushStyle, COLORREF color, ULONG_PTR hatch);
Brush newBrush(UINT brushStyle, COLORREF color, ULONG_PTR hatch);
	/* Note: these constructors are deprecated as they basically evaluate to a call to WinAPI function CreateBrushIndirect with the same parameters. They are to be replaced with more useful methods in the next versions. */
```



#Static functions

```C
/* Free the fields of a GUIObject */
void freeGUIObjectFields(GUIObject object);

/* Flush the current thread's message queue */
void flushMessageQueue();

/* Display a window with the application's command line settings */
BOOL displayWindow(Window mainWindow, int nCmdShow);

/* Display a control on its parent window */
BOOL displayControl(Control control);
```

#Other

```C
/* An event object */
struct _event {
  void (*eventFunction)(GUIObject, void*, EventArgs); /* The callback */
  enum _syncMode mode; /* The sync mode */
  UINT message; /* The message ID */
  GUIObject sender; /* The sender object */
  void *context; /* A pointer to data that gets sent on every event */
  EventArgs args; /* The event args */
  BOOL *condition; /* A pointer to a variable that determines if the event should be handled */
  BOOL interrupt; /* If this is set to TRUE, the default handling for the event doesn't occur */
  BOOL enabled; /* The event's enabled status */
};

/* An event handler callback type */
typedef void(*Callback)(GUIObject, void*, EventArgs);

/* Macros that expand to initPen (first 3) or initBrush (last 3) on a Pen or Brush object respectively with all parameters
   except for one being same as properties of the object */
_setPenStyle(penStyle)
_setPenWidth(width)
_setPenColor(color)
_setBrushStyle(brushStyle)
_setBrushColor(color)
_setBrushHatch(hatch)

```


That is all so far! Please send useful comments and constructive criticism to max.mints (at) mail.ru