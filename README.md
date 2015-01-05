tinyGUI
=======

A small and easy-to-use object-oriented Windows GUI library for C

Currently at version 0.2 (Alpha)

tinyGUI is a (tiny) object-oriented Windows GUI library for C. It was designed with ease of use being the primary concern.
tinyGUI sports a minimalist design, which contributes to its small size. However, all problems caused by abscence of certain functionality
from this or other versions is mitigated by easy extendibility.

To use tinyGUI, just add tinyGUI.h and tinyGUI.c to your project, 
`#include "tinyGUI/tinyGUI.h"`
and start coding!

tinyGUI is currently a work-in-progress, it is still missing many widgets and functionality that should make work easier.
A demonstration of some of its current capabilities can be found in demo.c. Stay tuned!

P.S. This is the first project I've ever hosted at GitHub, sorry if I've messed up somehow!

#Object - oriented system

tinyGUI uses a custom object-oriented system called tinyObject, designed to create objects that are as easy to use 
as to extend (though the former is the more important concern). This is achieved at the cost of slightly 
higher-intensive memory usage than that of different C object system implementations.

tinyObject uses the following syntaxis:

##Creating an object

```
Window window = newWindow(moduleInstance, "This is a test!", 440, 200); /* Create a window object of width 440 and height 200 */
Button button = newButton(moduleInstance, "Button", 100, 25, 10, 10); /* Create a button object of width 100, height 25, x position 10 and y position 10 */
```


 - Accessing an object’s field:

MessageBoxA(NULL, *window->text, "The window’s text!", MB_OK); /* Access an object’s field */

	Note: In tinyGUI, to ease using inherited fields (i.e. avoid the object->base.base.base.field - style inherited field accessing traditionally 
	associated with struct inheritance mechanisms in C), the fields are actually all pointers pointing to the real field values on the heap.
	The declarations of these pointers are duplicated between child and parent objects with the use of a macro (see tinyGUI/tinyGUI.h), 
	and the inherited pointers of a child object instance are to be set to point to the same memory as the corresponding pointers of its 
	parent instance. This slightly increases memory consumption, however, it makes using the objects easier.


-----------------------------------Calling an object’s method (way #1, less code but incurs a slight overhead)--

$(window)->setResizable(FALSE); /* Disable resizing a window */

	Note: to reduce the overhead, when calling several methods on the same object instance, use this syntax only on the
	first call, and perform all following calls as
	object->method(params);
	Remember that this will work only until you call a method on another object instance.

-------------------Calling an object’s method (way #2 – “traditional”, more code but incurs no overhead)---------

window->setResizableT(window, FALSE); /* Disable resizing a window – “traditional” */

	Note: all the methods are virtual, and object can override a parent's method. However, as the function pointers
	that represent the methods are duplicated between child and parent (just like with fields), but do not use the
	pointers-to-the-same-memory approach, and tinyObject doesn't use vtables, overriding a parent's method can be 
	slightly verbose (see example in the constructor of MouseEventArgs in tinyGUI/tinyGUI.c).


---------------------------------------Destroying an object-----------------------------

deleteWindow(window); /* Call a window’s destructor */


---------------------------------------Additional information---------------------------

tinyObject supports only public fields and virtual methods.
tinyObject has sealed classes. These are classes that do not implement the standard macro mechanisms used by base
classes for inheritance.






=======================================Library structure=======================================================

tinyGUI has, so far, the following class inheritance hierarchy:

 Object --> EventArgs --> MouseEventArgs
  | | |--> Pen
  | |--> Brush
  V
 GUIObject --> Window
  |
  |
  V
 Control --> Button
  | |
  | |--> TextBox
  V
 Label

A description of every class follows.


=======================================Class Object============================================================

Inheritance: base class

The main base class in tinyGUI, all the other classes directly or indirectly inherit from it.

---------------------------------------Fields------------------------------------------------------------------

enum _objectType *type; /* Identifies the object type */
CRITICAL_SECTION *criticalSection; /* The critical section for synchronising access to the object */

	Note: the second parameter should be initialized as a critical section (by calling InitializeCriticalSection(object->criticalSection)). 
	It can then be used anywhere in the code to synchronize access to an object. You can use the following utility macros for this:
	startSync(object)
	endSync(object)

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _object *Object;

/* The constructor prototype */
struct _object *newObject();

/* The destructor prototype */
void deleteObject(struct _object *object);


=======================================Class EventArgs=========================================================

Inheritance: base class, inherits from Object

Instances of this type are passed to event handler callbacks. They contain information about the Windows message
that triggered the event, along with the additional Windows parameters.

---------------------------------------Fields------------------------------------------------------------------

UINT *message; /* The Windows message */
WPARAM *wParam; /* The first Windows message additional parameter */
LPARAM *lParam; /* The second Windows message additional parameter */

---------------------------------------Methods-----------------------------------------------------------------

/* Updates the field values of the object corresponding to the parameters */
BOOL updateValue(UINT message, WPARAM wParam, LPARAM lParam);

	Note: although the "traditional" style method definitions are not given for clarity's sake, from here on, each described method 
	has a corresponding "traditional" method, which 1) has the letter T at the end of its declaration and 2) takes an object instance 
	as the first parameter.

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _eventargs *EventArgs;

/* The constructor prototype */
struct _eventargs *newEventArgs(UINT message, WPARAM wParam, LPARAM lParam);

/* The destructor prototype */
void deleteEventArgs(struct _eventargs *eventargs);


=======================================Class MouseEventArgs====================================================

Inheritance: sealed class, inherits from EventArgs

Instances of this type are passed to event handler callbacks when a mouse message is recieved.

---------------------------------------Fields------------------------------------------------------------------

int *cursorX; /* The X position of the cursor at the time the message was sent */
int *cursorY; /* The Y position of the cursor at the time the message was sent */

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _mouseeventargs *MouseEventArgs;

/* The constructor prototype */
struct _mouseeventargs *newMouseEventArgs(UINT message, WPARAM wParam, LPARAM lParam);

/* The destructor prototype */
void deleteMouseEventArgs(struct _mouseeventargs *eventargs);


=======================================Class GUIObject=========================================================

Inheritance: base class, inherits from Object

This is the main widget type; all the widgets directly or indirectly inherit from it.

---------------------------------------Fields------------------------------------------------------------------

LONG_PTR *origProcPtr; /* The pointer to the original window procedure */ 
	
HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */ 
HINSTANCE *moduleInstance; /* The current module instance */ 
PAINTSTRUCT *paintData; /* A structure with data about the GUIObject's painting */
HDC *paintContext; /* A handle to the window's current paint context. Initialized internally on receiving a WM_PAINT message */

char **className; /* The name of the window/control's WinAPI "class" */ 
HMENU *ID; /* The child-window/control identifier */ 
DWORD *styles; /* The window/control styles (WinAPI predefined macro values) */ 
DWORD *exStyles; /* The window/control extended styles (WinAPI predefined macro values) */ 

struct _event **events; /* An array of the GUIObject's events */
unsigned int *numEvents; /* The number of events registered for the GUIObject */

char **text; /* The window/control text */ 

int *width; /* The window/control width, pixels */
int *height; /* The window/control height, pixels */

int *minWidth; /* The window/control minimum width, pixels */
int *minHeight; /* The window/control minimum height, pixels */

int *maxWidth; /* The window/control maximum width, pixels */
int *maxHeight; /* The window/control maximum height, pixels */

int *realWidth; /* The window/control width used in anchor calculations. Not affected by min and max settings */
int *realHeight; /* The window/control height used in anchor calculations. Not affected by min and max settings */

int *x; /* The window/control x position (from left), pixels */
int *y; /* The window/control y position (from top), pixels */

int *realX; /*  used in anchor calculations. Not affected by min and max settings */
int *realY; /*  used in anchor calculations. Not affected by min and max settings */

BOOL *enabled; /* The GUIObject's enabled state */

---------------------------------------Methods-----------------------------------------------------------------

	Note: From here on, all BOOL methods return TRUE on success and FALSE on failure. I plan to replace this in the 
	future with an error-handling mechanism.

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
	Note: if the sender of an event is a type derived from GUIObject, or the event's arguments are of a type derived from EventArgs,
	it is preferred to cast them inside the function, along with the context parameter. However, you can also declare the callback with
	the actual expected types, like this, for example:
	void onClick(Button sender, Window mainWindow, MouseEventArgs e)
	and cast its pointer to the appropriate callback type (you can use the Callback typedef):
	$(button)->setOnClick((Callback)onClick, (void*)mainWindow, SYNC);
	Beware! This may be easier for you to use, but this technically causes undefined behaviour, according to the C standard, as this means
	that tinyGUI will call a function pointer typecasted from an incompatible function pointer type. However, in practice, in most modern 
	compilers on most modern systems, this is nearly always safe.

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
int setOnClick(void(*callback)(struct _guiobject*, void*, struct _eventargs*), void *context, enum _syncMode mode);

/* Moves a GUIObject to a new location specified by x and y */
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
   parameter pen */
BOOL drawLine(Pen pen, int x1, int y1, int x2, int y2);

/* Draws an arc in a GUIObject inside the bounds of a rectangle specified by boundX1, boundY1, boundX2, and boundY2
   (the upper left and the bottom right corners of the rectangle, respectively), from a point specified by x1 and y1 to
   the point specified by x2, y2 with a pen specified by the parameter pen */
BOOL drawArc(Pen pen, int boundX1, int boundY1, int boundX2, int boundY2, int x1, int y1, int x2, int y2);

/* Draws a rectangle in a GUIObject specified by boundX1, boundY1, boundX2, and boundY2 (the upper left and the bottom right corners of the 
   rectangle, respectively) with a pen specified by the parameter pen and fills it with the brush specified by the parameter brush. 
   If it is NULL, a hollow brush is used */
BOOL drawRect(Pen pen, Brush brush, int boundX1, int boundY1, int boundX2, int boundY2);

/* Draws a rounded rectangle in a GUIObject specified by boundX1, boundY1, boundX2, and boundY2 (the upper left and the bottom right 
   corners of the rectangle, respectively) with an ellipse of width ellipseWidth and height ellipseHeight being used 
   to draw the rounded edges, with a pen specified by the parameter pen, and fills it with the brush specified by the parameter brush. 
   If it is NULL, a hollow brush is used */
BOOL drawRoundedRect(Pen pen, Brush brush, int boundX1, int boundY1, 
							int boundX2, int boundY2, int ellipseWidth, int ellipseHeight);

/* Draws an ellipse in a GUIObject inside the bounds of a rectangle specified by boundX1, boundY1, boundX2, and boundY2 
   (the upper left and the bottom right corners of the rectangle, respectively) with a pen specified by the parameter pen 
   and fills it with the brush specified by the parameter brush. If it is NULL, a hollow brush is used */
BOOL drawEllipse(Pen pen, Brush brush, int boundX1, int boundY1, int boundX2, int boundY2);

/* Draws a polygon in a GUIObject between a set of points (number given by numPoints) specified by the coords array, which should 
   contain the points in the format {x1, y1, x2, y2, x3, y3, ... xn, yn}, with a pen specified by the parameter pen 
   and fills it with the brush specified by the parameter brush. If it is NULL, a hollow brush is used */
BOOL drawPolygon(Pen pen, Brush brush, int numPoints, LONG *coords);

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _guiobject *GUIObject;

/* The constructor prototype. The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the GUIObject's text, width and height specify its initial size */
struct _guiobject *newGUIObject(HINSTANCE instance, char *caption, int width, int height);

/* The destructor prototype */
void deleteGUIObject(struct _guiobject *object);


=======================================Class Window============================================================

Inheritance: sealed class, inherits from GUIObject

This class represents a window.

---------------------------------------Fields------------------------------------------------------------------

int *clientWidth; /* The client area's width, in pixels */
int *clientHeight; /* The client area's height, in pixels */

BOOL *resizable; /* The window's resizable state */
BOOL *maximizeEnabled; /* The window's maximize box state */

---------------------------------------Methods-----------------------------------------------------------------

/* Changes a window's resizable style */
BOOL setResizable(BOOL resizable);

/* Enables or disables a window's maximize box */
BOOL enableMaximize(BOOL maximize);

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _window *Window;

/* The constructor prototype. The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the window's text, width and height specify its initial size */
struct _window *newWindow(HINSTANCE instance, char *caption, int width, int height);

/* The destructor prototype */
void deleteWindow(struct _window *window);


=======================================Class Control===========================================================

Inheritance: base class, inherits from GUIObject

This is the base class for all controls.

---------------------------------------Fields------------------------------------------------------------------

int *minX; /* The minimum x position of the control (distance from parent's left edge), pixels */
int *minY; /* The minimum y position of the control (distance from parent's top edge), pixels */

int *maxX; /* The maximum x position of the control (distance from parent's left edge), pixels */
int *maxY; /* The maximum y position of the control (distance from parent's top edge), pixels */

short *anchor; /* The anchor settings for the control. It can be a bitwise addition (OR) of the following values:
				  ANCHOR_LEFT (0xF000), ANCHOR_RIGHT (0x000F), ANCHOR_TOP (0x0F00), ANCHOR_BOTTOM (0x00F0).
				  If an anchor is used, the control is moved or resized so that its borders remain at a constant
				  distance from the respective edges of its parent (limited by minWidth, minHeight, maxWidth, maxHeight,
				  minX, minY, maxX and maxY settings for the control). If neither of the two anchors for an orientation
				  (horizontal or vertical) are specified, the control is anchored to the center of its parent in this orientation. */

---------------------------------------Methods-----------------------------------------------------------------

/* Sets a control's minimum position to a new value specified by minX and minY */
BOOL setMinPos(int minX, int minY);

/* Sets a control's maximum position to a new value specified by maxX and maxY */
BOOL setMaxPos(int maxX, int maxY);

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _control *Control;

/* The constructor prototype. The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the control's text, width and height specify its initial size,
   x and y specify its initial position */
struct _control *newControl(HINSTANCE instance, char *caption, int width, int height, int x, int y);

/* The destructor prototype */
void deleteControl(struct _control *control);


=======================================Class Button============================================================

Inheritance: sealed class, inherits from Control

This class represents a button.

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _button *Button;

/* The constructor prototype. The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the button's text, width and height specify its initial size,
   x and y specify its initial position */
struct _button *newButton(HINSTANCE instance, char *caption, int width, int height, int x, int y);

/* The destructor prototype */
void deleteButton(struct _button *button);


=======================================Class TextBox===========================================================

Inheritance: sealed class, inherits from Control

This class represents a textbox (text input field).

---------------------------------------Fields------------------------------------------------------------------

BOOL *multiline; /* The textbox's multiline style */
BOOL *numOnly; /* The textbox's number only style - if it accepts only numbers or not */

---------------------------------------Methods-----------------------------------------------------------------

/* Sets the text input mode for a textbox to number-only or to not number-only */
BOOL setNumOnly(BOOL numOnly);

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _textbox *TextBox;

/* The constructor prototype. The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the textbox's text, width and height specify its initial size,
   x and y specify its initial position. The parameter multiline specifies the textbox's multiline style and can have the values
   MULTILINE and SINGLELINE */
struct _textbox *newTextBox(HINSTANCE instance, char *caption, int width, int height, int x, int y, enum _textboxtype multiline);

/* The destructor prototype */
void deleteTextBox(struct _textbox *textbox);


=======================================Class Label=============================================================

Inheritance: sealed class, inherits from Control

This class represents a label (static text field).

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _label *Label;

/* The constructor prototype. The parameter instance is the module instance of your executable, it can be obtained from the
   hInstance parameter of the WinMain function. Caption specifies the label's text, width and height specify its initial size,
   x and y specify its initial position */
struct _label *newLabel(HINSTANCE instance, char *caption, int width, int height, int x, int y);

/* The destructor prototype */
void deleteLabel(struct _label *label);


=======================================Class Pen===============================================================

Inheritance: sealed class, inherits from Object

This class represents a logical pen that can be used to draw lines.

---------------------------------------Fields------------------------------------------------------------------

HPEN *handle; /* The pen's handle */
int *penStyle; /* The pen's style (WinAPI predefined macro values) */
int *width; /* The pen's width/thickness */
COLORREF *color; /* The pen's color in RGB (can be defined with the RGB(r, g, b) WinAPI macro) */

---------------------------------------Methods-----------------------------------------------------------------

/* Updates the values of a Pen object's fields */
BOOL updateValue(int penStyle, int width, COLORREF color);

	Note: this basically evaluates to a call to WinAPI function CreatePen with the same parameters. As such, it is
	(along with the constructor) deprecated and is to be replaced with more useful methods in the next versions.

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _pen *Pen;

/* The constructor prototype. Sets the field values to the values of the respective parameters */
struct _pen *newPen(int penStyle, int width, COLORREF color);
	Note: this constructor is deprecated, see above.

/* The destructor prototype */
void deletePen(struct _pen *pen);


=======================================Class Brush=============================================================

Inheritance: sealed class, inherits from Object

This class represents a logical brush that can be used to fill areas.

---------------------------------------Fields------------------------------------------------------------------

HBRUSH *handle; /* The brush's handle */
UINT *brushStyle; /* The brush's style (WinAPI predefined macro values) */
COLORREF *color; /* The brush's color in RGB (can be defined with the RGB(r, g, b) WinAPI macro) */
ULONG_PTR *hatch; /* Either a predefined macro value of a WinAPI hatch style, or a handle to a bitmap with a pattern */

---------------------------------------Methods-----------------------------------------------------------------

/* Updates the value of a Brush object */
BOOL updateValue(UINT brushStyle, COLORREF color, ULONG_PTR hatch)

	Note: this basically evaluates to a call to WinAPI function CreateBrushIndirect with the same parameters. As such, it is
	(along with the constructor) deprecated and is to be replaced with more useful methods in the next versions.

---------------------------------------Other: typedef, constructor and destructor------------------------------

/* The class typedef */
typedef struct _brush *Brush;

/* The constructor prototype. Sets the field values to the values of the respective parameters */
struct _brush *newBrush(UINT brushStyle, COLORREF color, ULONG_PTR hatch);
	Note: this constructor is deprecated, see above.

/* The destructor prototype */
void deleteBrush(struct _brush *brush);




=======================================Static functions========================================================

/* Display a window with the application's command line settings */
BOOL displayWindow(Window mainWindow, int nCmdShow);

/* Display a control on its parent window */
BOOL displayControl(Control control);



That is all so far! Please send useful comments and constructive criticism to max.mints (at) mail.ru