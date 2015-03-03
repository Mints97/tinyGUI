#include <windows.h>
#include <windowsx.h>
#include <wincodec.h>
#include <stdio.h>

/* Set the visual styles for code compiling in Visual Studio */
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' \
							name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
							processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/* Static assertion - produces error with a message at compile time */
#define STATIC_ASSERT(condition, message) extern char STATIC_ASSERTION__##message[1]; \
    extern char STATIC_ASSERTION__##message[(condition)? 1 : 2]

/* tinyObject utility macros */
#define MAKE_FIELD(type, name, val) type name
#define DEFINE_FIELD(type, name, val)
#define INIT_FIELD(type, name, val) thisObject->name = (type)val

#define PRIVATE static

#define METHOD(classType, type, name, args) type classType##_##name args

#define MAKE_VIRTUAL_METHOD(classType, type, name, args) type (*name)args
#define INIT_VIRTUAL_METHOD(classType, type, name, args) thisObject->name = &classType##_##name

/* Determine if the compiler uses the regular struct member alignment logic, otherwise, inheritance won't work */
struct _ASSERTION_TEST_STRUCT { char a; int b; };
struct _ASSERTION_TEST_STRUCT2 { char a; int b; long c[100000]; long d[100000]; };
STATIC_ASSERT(offsetof(struct _ASSERTION_TEST_STRUCT, b) == offsetof(struct _ASSERTION_TEST_STRUCT2, b),
				This_compiler_follows_an_irregular_logic_in_struct_member_alignment_and_cannot_be_used_to_compile_tinyObject);

#define MAKE_THIS(type) type thisObject
#define ALLOC_THIS(type) struct type##_s *thisObject = (struct type##_s*)malloc(sizeof(struct type##_s))
#define this thisObject

#define MAKE_TYPEDEF(type) \
	typedef struct type##_s val_##type, *type; \

#define MAKE_CLASS(type) \
	struct type##_s { CLASS_##type }; \
	void delete##type (struct type##_s *thisObject)

#define MAKE_CLASS_TYPEDEFED(type) \
	MAKE_TYPEDEF(type); \
	MAKE_CLASS(type);

#define MAKE_DEFAULT_CONSTRUCTOR_PROTOTYPES(type) \
	struct type##_s *new##type(); \
	void init##type(struct type##_s *thisObject);

#define MAKE_DEFAULT_INITIALIZER(type) void init##type(struct type##_s *thisObject){ CLASS_##type }

#define MAKE_DEFAULT_CONSTRUCTOR(type) \
	struct type##_s *new##type(){ \
		struct type##_s *thisObject = (struct type##_s*)malloc(sizeof(struct type##_s)); \
		if (!thisObject) return NULL; \
		CLASS_##type \
		return thisObject; \
	}

#define MAKE_DEFAULT_CONSTRUCTORS(type) \
	MAKE_DEFAULT_INITIALIZER(type) \
	MAKE_DEFAULT_CONSTRUCTOR(type)

#define MAKE_DEFAULT_DESTRUCTOR(type) void delete##type(struct type##_s *thisObject){ free(thisObject); }

#define MAKE_DEFAULT_DESTRUCTOR_PROTOTYPE(type) void delete##type(struct type##_s *thisObject);

#define SELFREF_INIT void *currThis
#define $(object) (((currThis = (void*)(object)), object)
#define _(object) ((object)

#define CURR_THIS(type) (struct type##_s*)currThis
#define MAKE_METHOD_ALIAS(type, method) , type##_##method )
#define MAKE_VIRTUAL_METHOD_ALIAS(method) ->method )




/* tinyGUI utility macros */

/* Anchor macros */
#define ANCHOR_LEFT 0xF000
#define ANCHOR_RIGHT 0x000F
#define ANCHOR_TOP 0x0F00
#define ANCHOR_BOTTOM 0x00F0

/* Color macros */
#define COLOR_RED RGB(0xFF, 0, 0)
#define COLOR_GREEN RGB(0, 0xFF, 0)
#define COLOR_BLUE RGB(0, 0, 0xFF)

/* Synchronize access to current object */
#define startSync(object) EnterCriticalSection(&(object->criticalSection))
#define endSync(object) LeaveCriticalSection(&(object->criticalSection))



/* Datatypes */

/* The object types */
enum _objectType {
	OBJECT,
	GUIOBJECT,
	/* Window types */
	WINDOW,
	DIALOG,
	/* Control types */
	CONTROL,
	BUTTON,
	LABEL,
	TEXTBOX,
	/* Event arg types */
	EVENTARGS,
	MOUSEEVENTARGS,
	/* GDI types */
	PEN,
	BRUSH
};

/* An event sync mode */
enum _syncMode {
	SYNC,
	ASYNC
};

/* TextBox multiline/singleline */
enum _textboxtype {
	MULTILINE = TRUE,
	SINGLELINE = FALSE
};

/* An event */
typedef struct _event Event;



MAKE_TYPEDEF(Object);
MAKE_TYPEDEF(EventArgs);
MAKE_TYPEDEF(MouseEventArgs);
MAKE_TYPEDEF(GUIObject);
MAKE_TYPEDEF(Window);
MAKE_TYPEDEF(Control);
MAKE_TYPEDEF(Button);
MAKE_TYPEDEF(TextBox);
MAKE_TYPEDEF(Label);
MAKE_TYPEDEF(Pen);
MAKE_TYPEDEF(Brush);



/* Class Object */
#define CLASS_Object \
	FIELD(enum _objectType, type, OBJECT); \
	DEF_FIELD(CRITICAL_SECTION, criticalSection); \
	FIELD(BOOL, criticalSectionInitialized, FALSE);



/* Class EventArgs */
#define CLASS_EventArgs /* inherits from */ CLASS_Object \
	DEF_FIELD(UINT, message); \
	DEF_FIELD(WPARAM, wParam); \
	DEF_FIELD(LPARAM, lParam); \
	DEF_FIELD(void, (*updateValue)(MAKE_THIS(EventArgs), UINT message, WPARAM wParam, LPARAM lParam));


/* Class MouseEventArgs */
#define CLASS_MouseEventArgs /* inherits from */ CLASS_EventArgs \
	FIELD(int, cursorX, NULL); \
	FIELD(int, cursorY, NULL);


/* Class GUIObject */
#define CLASS_GUIObject /* inherits from */ CLASS_Object \
	FIELD(LONG_PTR, origProcPtr, NULL); /* The pointer to the original window procedure */  \
	\
	FIELD(HWND, handle, NULL); /* The handle to the window/control; initialized with a call to CreateWindowEx */  \
	FIELD(HINSTANCE, moduleInstance, NULL); /* The current module instance */  \
	DEF_FIELD(PAINTSTRUCT, paintData); \
	FIELD(HDC, paintContext, NULL); \
	FIELD(HDC, offscreenPaintContext, NULL); \
	FIELD(HBITMAP, offscreenBitmap, NULL); \
	FIELD(BOOL, customEraseBG, FALSE); \
	\
	FIELD(char*, className, NULL); /* The name of the window/control's WinAPI "class" */  \
	FIELD(HMENU, ID, 0); /* The child-window/control identifier */  \
	FIELD(DWORD, styles, 0); /* The window/control styles */  \
	FIELD(DWORD, exStyles, 0); /* The window/control extended styles */  \
	\
	/* events */  \
	FIELD(struct _event*, events, NULL); \
	FIELD(unsigned int, numEvents, 0); \
	\
	FIELD(char*, text, NULL); /* The window/control text */  \
	\
	FIELD(int, width, 0); \
	FIELD(int, height, 0); \
	\
	FIELD(int, minWidth, 0); \
	FIELD(int, minHeight, 0); \
	\
	FIELD(int, maxWidth, INT_MAX); \
	FIELD(int, maxHeight, INT_MAX); \
	\
	FIELD(int, realWidth, 0); /* Used in anchor calculations. Not affected by min and max settings */ \
	FIELD(int, realHeight, 0); \
	\
	FIELD(int, x, CW_USEDEFAULT); \
	FIELD(int, y, CW_USEDEFAULT); \
	\
	FIELD(int, realX, CW_USEDEFAULT); /* Used in anchor calculations. Not affected by min and max settings */ \
	FIELD(int, realY, CW_USEDEFAULT); \
	\
	FIELD(BOOL, enabled, TRUE); \
	\
	/* links */  \
	FIELD(GUIObject, parent, NULL); /* A pointer to the parent */  \
	FIELD(GUIObject*, children, NULL); /* A pointer to the array of children */  \
	FIELD(unsigned int, numChildren, 0); /* The number of children */  \
	\
	/* Moves a GUIObject to a new location specified by x and y */ \
	VIRTUAL_METHOD(GUIObject, BOOL, setPos, (MAKE_THIS(GUIObject), int x, int y));

	/* Adds a child object to a GUIObject */
	METHOD(GUIObject, BOOL, addChild, (MAKE_THIS(GUIObject), struct GUIObject_s *child));
	/* Removes a child object from a GUIObject */
	METHOD(GUIObject, BOOL, removeChild, (MAKE_THIS(GUIObject), struct GUIObject_s *child));
	/* Sets an event for a GUIObject by a Windows message */
	METHOD(GUIObject, int, setEvent, (MAKE_THIS(GUIObject), DWORD message, void(*callback)(GUIObject, void*, struct EventArgs_s*),
						 void *context, enum _syncMode mode));
	/* Sets a condition for an event of a GUIObject */
	METHOD(GUIObject, BOOL, setEventCondition, (MAKE_THIS(GUIObject), int eventID, BOOL *condition));
	/* Sets an interrupt state for an event of a GUIObject */
	METHOD(GUIObject, BOOL, setEventInterrupt, (MAKE_THIS(GUIObject), int eventID, BOOL interrupt));
	/* Changes an event's enabled state for a GUIObject */
	METHOD(GUIObject, BOOL, setEventEnabled, (MAKE_THIS(GUIObject), int eventID, BOOL enabled));
	/* Sets a WM_LBUTTONUP event for an (enabled) object */
	METHOD(GUIObject, int, setOnClick, (MAKE_THIS(GUIObject), void(*callback)(GUIObject, void*, struct EventArgs_s*), void *context,
											enum _syncMode mode));
	/* Resizes a GUIObject to a new size specified by width and height */
	METHOD(GUIObject, BOOL, setSize, (MAKE_THIS(GUIObject), int width, int height));
	/* Sets a GUIObject's minimum size to a new value specified by minWidth and minHeight */
	METHOD(GUIObject, BOOL, setMinSize, (MAKE_THIS(GUIObject), int minWidth, int minHeight));
	/* Sets a GUIObject's maximum size to a new value specified by maxWidth and maxHeight */
	METHOD(GUIObject, BOOL, setMaxSize, (MAKE_THIS(GUIObject), int maxWidth, int maxHeight));
	/* Sets a new text for a GUIObject */
	METHOD(GUIObject, BOOL, setText, (MAKE_THIS(GUIObject), char *text));
	/* Sets a GUIObject's enabled state */
	METHOD(GUIObject, BOOL, setEnabled, (MAKE_THIS(GUIObject), BOOL enabled));
	/* Draws a line in a GUIObject from the point specified by x1, y1 to the point specified by x2, y2 */
	METHOD(GUIObject, BOOL, drawLine, (MAKE_THIS(GUIObject), Pen pen, int x1, int y1, int x2, int y2));
	/* Draws an arc in a GUIObject */
	METHOD(GUIObject, BOOL, drawArc, (MAKE_THIS(GUIObject), Pen pen, int boundX1, int boundY1, int boundX2, int boundY2, 
											int x1, int y1, int x2, int y2));
	/* Draws a rectangle in a GUIObject */
	METHOD(GUIObject, BOOL, drawRect, (MAKE_THIS(GUIObject), Pen pen, Brush brush, int boundX1, int boundY1, 
											int boundX2, int boundY2));
	/* Draws a rounded rectangle in a GUIObject */
	METHOD(GUIObject, BOOL, drawRoundedRect, (MAKE_THIS(GUIObject), Pen pen, Brush brush, int boundX1, int boundY1, 
												int boundX2, int boundY2, int ellipseWidth, int ellipseHeight));
	/* Draws an ellipse in a GUIObject */
	METHOD(GUIObject, BOOL, drawEllipse, (MAKE_THIS(GUIObject), Pen pen, Brush brush, int boundX1, int boundY1, 
												int boundX2, int boundY2));
	/* Draws a polygon in a GUIObject */
	METHOD(GUIObject, BOOL, drawPolygon, (MAKE_THIS(GUIObject), Pen pen, Brush brush, int numPoints, LONG *coords));

	/* Virtual method prototype */
	BOOL GUIObject_setPos(GUIObject object, int x, int y);

	/* Self-reference mechanism for methods */
	/* Moves a GUIObject to a new location specified by x and y */
	#define _setPos(x, y) MAKE_VIRTUAL_METHOD_ALIAS(setPos(CURR_THIS(GUIObject), x, y))

	#define _addChild(child) MAKE_METHOD_ALIAS(GUIObject, addChild(CURR_THIS(GUIObject), child))
	/* Removes a child object from a GUIObject */
	#define _removeChild(child) MAKE_METHOD_ALIAS(GUIObject, removeChild(CURR_THIS(GUIObject), child))
	/* Sets an event for a GUIObject by a Windows message */
	#define _setEvent(message, callback, context, mode) MAKE_METHOD_ALIAS(GUIObject, setEvent(CURR_THIS(GUIObject), message, callback, context, mode))
	/* Sets a condition for an event of a GUIObject */
	#define _setEventCondition(object, eventID, condition) MAKE_METHOD_ALIAS(GUIObject, setEventCondition(CURR_THIS(GUIObject), eventID, condition))
	/* Sets an interrupt state for an event of a GUIObject */
	#define _setEventInterrupt(eventID, interrupt) MAKE_METHOD_ALIAS(GUIObject, setEventInterrupt(CURR_THIS(GUIObject), eventID, interrupt))
	/* Changes an event's enabled state for a GUIObject */
	#define _setEventEnabled(eventID, enabled) MAKE_METHOD_ALIAS(GUIObject, setEventEnabled(CURR_THIS(GUIObject), eventID, enabled))
	/* Sets a WM_LBUTTONUP event for an (enabled) object */
	#define _setOnClick(callback, context, mode) MAKE_METHOD_ALIAS(GUIObject, setOnClick(CURR_THIS(GUIObject), callback, context, mode))
	/* Resizes a GUIObject to a new size specified by width and height */
	#define _setSize(width, height) MAKE_METHOD_ALIAS(GUIObject, setSize(CURR_THIS(GUIObject), width, height))
	/* Sets a GUIObject's minimum size to a new value specified by minWidth and minHeight */
	#define _setMinSize(minWidth, minHeight) MAKE_METHOD_ALIAS(GUIObject, setMinSize(CURR_THIS(GUIObject), minWidth, minHeight))
	/* Sets a GUIObject's maximum size to a new value specified by maxWidth and maxHeight */
	#define _setMaxSize(maxWidth, maxHeight) MAKE_METHOD_ALIAS(GUIObject, setMaxSize(CURR_THIS(GUIObject), maxWidth, maxHeight))
	/* Sets a new text for a GUIObject */
	#define _setText(text) MAKE_METHOD_ALIAS(GUIObject, setText(CURR_THIS(GUIObject), text))
	/* Sets a GUIObject's enabled state */
	#define _setEnabled(enabled) MAKE_METHOD_ALIAS(GUIObject, setEnabled(CURR_THIS(GUIObject), enabled))
	/* Draws a line in a GUIObject from the pospecified by x1, y1 to the pospecified by x2, y2 */
	#define _drawLine(pen, x1, y1, x2, y2) MAKE_METHOD_ALIAS(GUIObject, drawLine(CURR_THIS(GUIObject), pen, x1, y1, x2, y2))
	/* Draws an arc in a GUIObject */
	#define _drawArc(pen, boundX1, boundY1, boundX2, boundY2, x1, y1, x2, y2) MAKE_METHOD_ALIAS(GUIObject, \
																drawArc(CURR_THIS(GUIObject), pen, boundX1, boundY1, boundX2, boundY2, x1, y1, x2, y2))
	/* Draws a rectangle in a GUIObject */
	#define _drawRect(pen, brush, boundX1, boundY1, boundX2, boundY2) MAKE_METHOD_ALIAS(GUIObject, \
																		drawRect(CURR_THIS(GUIObject), pen, brush, boundX1, boundY1, boundX2, boundY2))
	/* Draws a rounded rectangle in a GUIObject */
	#define _drawRoundedRect(pen, brush, boundX1, boundY1, boundX2, boundY2, ellipseWidth, ellipseHeight) MAKE_METHOD_ALIAS(GUIObject, \
									drawRoundedRect(CURR_THIS(GUIObject), pen, brush, boundX1, boundY1, boundX2, boundY2, ellipseWidth, ellipseHeight)
	/* Draws an ellipse in a GUIObject */
	#define _drawEllipse(pen, brush, boundX1, boundY1, boundX2, boundY2) MAKE_METHOD_ALIAS(GUIObject, \
																	drawEllipse(CURR_THIS(GUIObject), pen, brush, boundX1, boundY1, boundX2, boundY2))
	/* Draws a polygon in a GUIObject */
	#define _drawPolygon(pen, brush, numPoints, coords) MAKE_METHOD_ALIAS(GUIObject, \
																					drawPolygon(CURR_THIS(GUIObject), pen, brush, numPoints, coords))



/* Class Window */
#define CLASS_Window /* inherits from */ CLASS_GUIObject \
	/* fields */ \
	FIELD(int, clientWidth, 0); \
	FIELD(int, clientHeight, 0); \
	\
	FIELD(BOOL, resizable, TRUE); \
	FIELD(BOOL, maximizeEnabled, TRUE);

	/* methods */
	METHOD(Window, BOOL, setResizable, (MAKE_THIS(Window), BOOL resizable));
	METHOD(Window, BOOL, enableMaximize, (MAKE_THIS(Window), BOOL maximize));

	/* Self-reference mechanism for methods */
	#define _setResizable(resizable) MAKE_METHOD_ALIAS(Window, setResizable(CURR_THIS(Window), resizable))
	#define _enableMaximize(maximize) MAKE_METHOD_ALIAS(Window, enableMaximize(CURR_THIS(Window), maximize))



/* Class Control */
#define CLASS_Control /* inherits from */ CLASS_GUIObject \
	/* fields */ \
	FIELD(int, minX, INT_MIN); \
	FIELD(int, minY, INT_MIN); \
	\
	FIELD(int, maxX, INT_MAX); \
	FIELD(int, maxY, INT_MAX); \
	\
	FIELD(short, anchor, ANCHOR_TOP | ANCHOR_LEFT);


	/* methods */
	METHOD(Control, BOOL, setMinPos, (MAKE_THIS(Control), int minX, int minY));
	METHOD(Control, BOOL, setMaxPos, (MAKE_THIS(Control), int maxX, int maxY));

	/* Self-reference mechanism for methods */
	#define _setMinPos(minX, minY) MAKE_METHOD_ALIAS(Control, setMinPos(CURR_THIS(Control), minX, minY))
	#define _setMaxPos(maxX, maxY) MAKE_METHOD_ALIAS(Control, setMaxPos(CURR_THIS(Control), maxX, maxY))



/* Class Button */
#define CLASS_Button /* inherits from */ CLASS_Control



/* Class TextBox */
#define CLASS_TextBox /* inherits from */ CLASS_Control \
	FIELD(BOOL, multiline, FALSE); \
	FIELD(BOOL, numOnly, FALSE);

	METHOD(TextBox, BOOL, setNumOnly, (MAKE_THIS(TextBox), BOOL numOnly));

	#define _setNumOnly(numOnly) MAKE_METHOD_ALIAS(TextBox, setNumOnly(CURR_THIS(TextBox), numOnly))



/* Class Label */
#define CLASS_Label /* inherits from */ CLASS_Control



/* Class Pen */
#define CLASS_Pen /* inherits from */ CLASS_Object \
	FIELD(HPEN, handle, NULL); \
	FIELD(int, penStyle, 0); \
	FIELD(int, width, 0); \
	FIELD(COLORREF, color, 0);

	#define _setPenStyle(penStyle) , initPen(CURR_THIS(Pen), penStyle, (CURR_THIS(Pen))->width, (CURR_THIS(Pen))->color))
	#define _setPenWidth(width) , initPen(CURR_THIS(Pen), (CURR_THIS(Pen))->penStyle, width, (CURR_THIS(Pen))->color))
	#define _setPenColor(color) , initPen(CURR_THIS(Pen), (CURR_THIS(Pen))->penStyle, (CURR_THIS(Pen))->width, color))



/* Class Brush */
#define CLASS_Brush /* inherits from */ CLASS_Object \
	FIELD(HBRUSH, handle, NULL); \
	FIELD(UINT, brushStyle, 0); \
	FIELD(COLORREF, color, 0); \
	FIELD(ULONG_PTR, hatch, NULL);

	#define _setBrushStyle(brushStyle) , initBrush(CURR_THIS(Brush), brushStyle, (CURR_THIS(Pen))->color, (CURR_THIS(Brush))->hatch))
	#define _setBrushColor(color) , initBrush(CURR_THIS(Brush), (CURR_THIS(Brush))->brushStyle, color, (CURR_THIS(Brush))->hatch))
	#define _setBrushHatch(hatch) , initBrush(CURR_THIS(Brush), (CURR_THIS(Brush))->brushStyle, (CURR_THIS(Brush))->color, hatch))





/* Make the classes */
#define FIELD(type, name, val) MAKE_FIELD(type, name, val)
#define DEF_FIELD(type, name) MAKE_FIELD(type, name, )
#define VIRTUAL_METHOD(classType, type, name, args) MAKE_VIRTUAL_METHOD(classType, type, name, args)

	MAKE_CLASS(Object);
	MAKE_DEFAULT_CONSTRUCTOR_PROTOTYPES(Object);

	MAKE_CLASS(EventArgs);
	void initEventArgs(EventArgs thisObject, UINT message, WPARAM wParam, LPARAM lParam);
	EventArgs newEventArgs(UINT message, WPARAM wParam, LPARAM lParam);

	MAKE_CLASS(MouseEventArgs);
	void initMouseEventArgs(MouseEventArgs thisObject, UINT message, WPARAM wParam, LPARAM lParam);
	MouseEventArgs newMouseEventArgs(UINT message, WPARAM wParam, LPARAM lParam);

	MAKE_CLASS(Pen);
	void initPen(Pen thisObject, int penStyle, int width, COLORREF color);
	Pen newPen(int penStyle, int width, COLORREF color);

	MAKE_CLASS(Brush);
	void initBrush(Brush thisObject, UINT brushStyle, COLORREF color, ULONG_PTR hatch);
	Brush newBrush(UINT brushStyle, COLORREF color, ULONG_PTR hatch);

	MAKE_CLASS(GUIObject);
	void initGUIObject(GUIObject thisObject, HINSTANCE instance, char *text, int width, int height);
	GUIObject newGUIObject(HINSTANCE instance, char *text, int width, int height);

	MAKE_CLASS(Window);
	void initWindow(Window thisObject, HINSTANCE instance, char *text, int width, int height);
	Window newWindow(HINSTANCE instance, char *text, int width, int height);

	MAKE_CLASS(Control);
	void initControl(Control thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height);
	Control newControl(HINSTANCE instance, char *text, int x, int y, int width, int height);

	MAKE_CLASS(Button);
	void initButton(Button thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height);
	Button newButton(HINSTANCE instance, char *text, int x, int y, int width, int height);

	MAKE_CLASS(TextBox);
	void initTextBox(TextBox thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height, enum _textboxtype multiline);
	TextBox newTextBox(HINSTANCE instance, char *text, int x, int y, int width, int height, enum _textboxtype multiline);

	MAKE_CLASS(Label);
	void initLabel(Label thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height);
	Label newLabel(HINSTANCE instance, char *text, int x, int y, int width, int height);


#undef FIELD
#undef DEF_FIELD
#undef VIRTUAL_METHOD


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



/* "static" function prototypes */
static void *getCurrentThis();
void setCurrentThis(void *self);

void freeGUIObjectFields(GUIObject object);

void flushMessageQueue();

BOOL displayControl(Control control);
BOOL displayWindow(Window mainWindow, int nCmdShow);