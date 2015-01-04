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



/* ======================================================Class Object============================================================================= */
/* Inheritance: base class */
struct _object {
#define _object_MEMBERS \
	enum _objectType *type; /* The object type */ \
	CRITICAL_SECTION *criticalSection /* The critical section for synchronising access to the object */

	_object_MEMBERS;
};

/* To be used in constructors of any class directly derived from Object */
#define inheritFromObject(object, basePointer) \
		if (!basePointer) return NULL; \
		object->base = *basePointer; \
		object->type = object->base.type; \
		object->criticalSection = object->base.criticalSection

/* The class typedef */
typedef struct _object *Object;

/* The constructor prototype */
struct _object *newObject();

/* The destructor prototype */
void deleteObject(struct _object *object);



/* ======================================================Class EventArgs=========================================================================== */
/* Inheritance: base class, inherits from Object */
struct _eventargs {
	struct _object base;

#define _eventargs_MEMBERS \
	_object_MEMBERS; \
	/* fields */ \
	UINT *message; \
	WPARAM *wParam; \
	LPARAM *lParam; \
	/* methods */ \
	BOOL (*updateValue)(UINT, WPARAM, LPARAM); \
	BOOL (*updateValueT)(struct _eventargs*, UINT, WPARAM, LPARAM)

	_eventargs_MEMBERS;
};

/* To be used in constructors of any class directly derived from EventArgs */
#define inheritFromEventArgs(object, basePointer) \
		inheritFromObject(object, basePointer); \
		object->message = object->base.message; \
		object->wParam = object->base.wParam; \
		object->lParam = object->base.lParam; \
		object->updateValue = object->base.updateValue; \
		object->updateValueT = object->base.updateValueT

/* The class typedef */
typedef struct _eventargs *EventArgs;

/* The constructor prototype */
struct _eventargs *newEventArgs(UINT message, WPARAM wParam, LPARAM lParam);

/* The destructor prototype */
void deleteEventArgs(struct _eventargs *eventargs);



/* ======================================================Class MouseEventArgs====================================================================== */
/* Inheritance: sealed class, inherits from EventArgs */
struct _mouseeventargs {
	struct _eventargs base;

#define _mouseeventargs_MEMBERS
	_eventargs_MEMBERS; \
	int *cursorX; \
	int *cursorY

	_mouseeventargs_MEMBERS;
};

/* The class typedef */
typedef struct _mouseeventargs *MouseEventArgs;

/* The constructor prototype */
struct _mouseeventargs *newMouseEventArgs(UINT message, WPARAM wParam, LPARAM lParam);

/* The destructor prototype */
void deleteMouseEventArgs(struct _mouseeventargs *eventargs);



/* ====================================================Non - class types=========================================================================== */

/* An event */
struct _event {
	void (*eventFunction)(struct _guiobject*, void*, struct _eventargs*); /* The callback */
	enum _syncMode mode; /* The sync mode */
	UINT message; /* The message ID */
	struct _guiobject *sender; /* The sender object */
	void *context; /* A pointer to data that gets sent on every event */
	struct _eventargs *args; /* The event args */
	BOOL *condition; /* A pointer to a variable that determines if the event should be handled */
	BOOL interrupt; /* If this is set to TRUE, the default handling for the event doesn't occur */
	BOOL enabled; /* The event's enabled status */
};


/* ======================================================Class GUIObject=========================================================================== */
/* Inheritance: base class, inherits from Object */
struct _guiobject {
	struct _object base;

#define _guiobject_MEMBERS \
	_object_MEMBERS; \
	LONG_PTR *origProcPtr; /* The pointer to the original window procedure */  \
	 \
	HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */  \
	HINSTANCE *moduleInstance; /* The current module instance */  \
	PAINTSTRUCT *paintData; \
	HDC *paintContext; \
	 \
	char **className; /* The name of the window/control's WinAPI "class" */  \
	HMENU *ID; /* The child-window/control identifier */  \
	DWORD *styles; /* The window/control styles */  \
	DWORD *exStyles; /* The window/control extended styles */  \
	 \
	/* events */  \
	struct _event **events; \
	unsigned int *numEvents; \
	 \
	char **text; /* The window/control text */  \
	 \
	int *width; \
	int *height; \
	 \
	int *minWidth; \
	int *minHeight; \
	 \
	int *maxWidth; \
	int *maxHeight; \
	 \
	int *realWidth; /* Used in anchor calculations. Not affected by min and max settings */ \
	int *realHeight; \
	 \
	int *x; \
	int *y; \
	 \
	int *realX; /* Used in anchor calculations. Not affected by min and max settings */ \
	int *realY; \
	 \
	BOOL *enabled; \
	 \
	/* links */  \
	struct _guiobject **parent; /* A pointer to the parent */  \
	struct _guiobject ***children; /* A pointer to the array of children */  \
	unsigned int *numChildren; /* The number of children */  \
	 \
	/* methods */  \
	BOOL (*addChild)(struct _guiobject*); /* Add a child control to the object */ \
	BOOL (*addChildT)(struct _guiobject*, struct _guiobject*); /* Version without self-reference mechanism */ \
	 \
	BOOL (*removeChild)(struct _guiobject*); /* Remove a child control from the object */ \
	BOOL (*removeChildT)(struct _guiobject*, struct _guiobject*); /* Version without self-reference mechanism */ \
	 \
	int (*setEvent)(DWORD, void(*)(struct _guiobject*, void*, struct _eventargs*), void*, enum _syncMode); \
	int (*setEventT)(struct _guiobject*, DWORD, void(*)(struct _guiobject*, void*, struct _eventargs*), void*, enum _syncMode); \
	 \
	BOOL (*setEventCondition)(int, BOOL*); \
	BOOL (*setEventConditionT)(struct _guiobject*, int, BOOL*); \
	 \
	BOOL (*setEventInterrupt)(int, BOOL); \
	BOOL (*setEventInterruptT)(struct _guiobject*, int, BOOL); \
	 \
	BOOL (*setEventEnabled)(int, BOOL); \
	BOOL (*setEventEnabledT)(struct _guiobject*, int, BOOL); \
	 \
	int (*setOnClick)(void(*)(struct _guiobject*, void*, struct _eventargs*), void*, enum _syncMode); /* Set an OnClick event for the object */ \
	int (*setOnClickT)(struct _guiobject*, void(*)(struct _guiobject*, void*, struct _eventargs*), void*, enum _syncMode); /* Version without self-reference mechanism */ \
	 \
	BOOL (*setPos)(int, int); \
	BOOL (*setPosT)(struct _guiobject*, int, int); \
	 \
	BOOL (*setSize)(int, int); \
	BOOL (*setSizeT)(struct _guiobject*, int, int); \
	 \
	BOOL (*setMinSize)(int, int); \
	BOOL (*setMinSizeT)(struct _guiobject*, int, int); \
	 \
	BOOL (*setMaxSize)(int, int); \
	BOOL (*setMaxSizeT)(struct _guiobject*, int, int); \
	 \
	BOOL (*setText)(char *text); \
	BOOL (*setTextT)(struct _guiobject*, char *text); \
	 \
	BOOL (*setEnabled)(BOOL); \
	BOOL (*setEnabledT)(struct _guiobject*, BOOL); \
	 \
	BOOL (*drawLine)(struct _pen*, int, int, int, int); \
	BOOL (*drawLineT)(struct _guiobject*, struct _pen*, int, int, int, int); \
	 \
	BOOL (*drawArc)(struct _pen*, int, int, int, int, int, int, int, int); \
	BOOL (*drawArcT)(struct _guiobject*, struct _pen*, int, int, int, int, int, int, int, int); \
	 \
	/*BOOL (*drawText)(struct _pen*, int, int, int, int);*/ \
	/*BOOL (*drawTextT)(struct _guiobject*, struct _pen*, int, int, int, int);*/ \
	 \
	BOOL (*drawRect)(struct _pen*, struct _brush*, int, int, int, int); \
	BOOL (*drawRectT)(struct _guiobject*, struct _pen*, struct _brush*, int, int, int, int); \
	 \
	BOOL (*drawRoundedRect)(struct _pen*, struct _brush*, int, int, int, int, int, int); \
	BOOL (*drawRoundedRectT)(struct _guiobject*, struct _pen*, struct _brush*, int, int, int, int, int, int); \
	 \
	BOOL (*drawEllipse)(struct _pen*, struct _brush*, int, int, int, int); \
	BOOL (*drawEllipseT)(struct _guiobject*, struct _pen*, struct _brush*, int, int, int, int); \
	 \
	BOOL (*drawPolygon)(struct _pen*, struct _brush*, int, LONG*); \
	BOOL (*drawPolygonT)(struct _guiobject*, struct _pen*, struct _brush*, int, LONG*)

	_guiobject_MEMBERS;
	/* TODO (events): setOnSizeChange, setOnPosChange, setOnPaint, setOnRightclick */
};

/* To be used in constructors of any class directly derived from GUIObject */
#define inheritFromGUIObject(object, basePointer) \
	inheritFromObject(object, basePointer); \
	object->origProcPtr = object->base.origProcPtr; \
	object->handle = object->base.handle; object->moduleInstance = object->base.moduleInstance; \
	object->paintData = object->base.paintData; object->paintContext = object->base.paintContext; \
	object->className = object->base.className; \
	object->ID = object->base.ID; object->styles = object->base.styles; object->exStyles = object->base.exStyles; \
	/* event handling */ object->events = object->base.events; object->numEvents = object->base.numEvents; \
	/* fields */ object->text = object->base.text; \
	object->width = object->base.width; object->height = object->base.height; \
	object->realWidth = object->base.realWidth; object->realHeight = object->base.realHeight; \
	object->minWidth = object->base.minWidth; object->minHeight = object->base.minHeight; \
	object->x = object->base.x; object->y = object->base.y; object->realX = object->base.realX; object->realY = object->base.realY; \
	object->enabled = object->base.enabled; \
	/* links */ object->parent = object->base.parent; object->children = object->base.children; \
	object->numChildren = object->base.numChildren; \
	/* methods */ \
	object->addChild = object->base.addChild; object->addChildT = object->base.addChildT; \
	object->removeChild = object->base.removeChild; object->removeChildT = object->base.removeChildT; \
	object->setEvent = object->base.setEvent; object->setEventT = object->base.setEventT; \
	object->setEventCondition = object->base.setEventCondition; object->setEventConditionT = object->base.setEventConditionT; \
	object->setEventInterrupt = object->base.setEventInterrupt; object->setEventInterruptT = object->base.setEventInterruptT; \
	object->setEventEnabled = object->base.setEventEnabled; object->setEventEnabledT = object->base.setEventEnabledT; \
	object->setOnClick = object->base.setOnClick; object->setOnClickT = object->base.setOnClickT; \
	object->setPos = object->base.setPos; object->setPosT = object->base.setPosT; \
	object->setSize = object->base.setSize; object->setSizeT = object->base.setSizeT; \
	object->setMinSize = object->base.setMinSize; object->setMinSizeT = object->base.setMinSizeT; \
	object->setMaxSize = object->base.setMaxSize; object->setMaxSizeT = object->base.setMaxSizeT; \
	object->setText = object->base.setText; object->setTextT = object->base.setTextT; \
	object->setEnabled = object->base.setEnabled; object->setEnabledT = object->base.setEnabledT; \
	object->drawLine = object->base.drawLine; object->drawLineT = object->base.drawLineT; \
	object->drawArc = object->base.drawArc; object->drawArcT = object->base.drawArcT; \
	/*object->drawText = object->base.drawText; object->drawTextT = object->base.drawTextT;*/ \
	object->drawRect = object->base.drawRect; object->drawRectT = object->base.drawRectT; \
	object->drawRoundedRect = object->base.drawRoundedRect; object->drawRoundedRectT = object->base.drawRoundedRectT; \
	object->drawEllipse = object->base.drawEllipse; object->drawEllipseT = object->base.drawEllipseT; \
	object->drawPolygon = object->base.drawPolygon; object->drawPolygonT = object->base.drawPolygonT

/* The class typedef */
typedef struct _guiobject *GUIObject;

/* The constructor prototype */
struct _guiobject *newGUIObject(HINSTANCE instance, char *caption, int width, int height);

/* The destructor prototype */
void deleteGUIObject(struct _guiobject *object);



/* ======================================================Class Window============================================================================= */
/* Inheritance: sealed class, inherits from GUIObject */
struct _window {
	struct _guiobject base;

#define _window_MEMBERS
	_guiobject_MEMBERS; \
	/* fields */ \
	int *clientWidth; \
	int *clientHeight; \
	 \
	BOOL *resizable; \
	BOOL *maximizeEnabled; \
	 \
	/* methods */ \
	BOOL (*setResizable)(BOOL); \
	BOOL (*setResizableT)(struct _window*, BOOL); \
	 \
	BOOL (*enableMaximize)(BOOL); \
	BOOL (*enableMaximizeT)(struct _window*, BOOL)

	_window_MEMBERS;

	//TODO (events): setOnExit, setOnKeypress
	//TODO (other methods): setFullscreen, makeTopmost
};

/* The class typedef */
typedef struct _window *Window;

/* The constructor prototype */
struct _window *newWindow(HINSTANCE instance, char *caption, int width, int height);

/* The destructor prototype */
void deleteWindow(struct _window *window);



/* ======================================================Class Control============================================================================= */
/* Inheritance: base class, inherits from GUIObject */
struct _control {
	struct _guiobject base;

#define _control_MEMBERS \
	_guiobject_MEMBERS; \
	 \
	/* fields */ \
	int *minX; \
	int *minY; \
	 \
	int *maxX; \
	int *maxY; \
	 \
	short *anchor; \
	 \
	/* methods */ \
	BOOL (*setMinPos)(int, int); \
	BOOL (*setMinPosT)(struct _control*, int, int); \
	 \
	BOOL (*setMaxPos)(int, int); \
	BOOL (*setMaxPosT)(struct _control*, int, int)

	//TODO: transparency!

	_control_MEMBERS;
};

/* To be used in constructors of any class directly derived from Control */
#define inheritFromControl(object, basePointer) \
		inheritFromGUIObject(object, basePointer); \
		object->minX = object->base.minX; object->minY = object->base.minY; \
		object->maxX = object->base.maxX; object->maxY = object->base.maxY; \
		object->anchor = object->base.anchor

/* The class typedef */
typedef struct _control *Control;

/* The constructor prototype */
struct _control *newControl(HINSTANCE instance, char *caption, int width, int height, int x, int y);

/* The destructor prototype */
void deleteControl(struct _control *control);



/* ======================================================Class Button============================================================================= */
/* Inheritance: sealed class, inherits from Control */
struct _button {
	struct _control base;

#define _button_MEMBERS \
	_control_MEMBERS \
	//TODO?

	_button_MEMBERS;
};

/* The class typedef */
typedef struct _button *Button;

/* The constructor prototype */
struct _button *newButton(HINSTANCE instance, char *caption, int width, int height, int x, int y);

/* The destructor prototype */
void deleteButton(struct _button *button);



/* ======================================================Class TextBox============================================================================= */
/* Inheritance: sealed class, inherits from Control */
struct _textbox {
	struct _control base;

#define _textbox_MEMBERS \
	_control_MEMBERS; \
	/* fields */ \
	BOOL *multiline; \
	BOOL *numOnly; \
	 \
	/* methods */ \
	BOOL (*setNumOnly)(BOOL); \
	BOOL (*setNumOnlyT)(struct _textbox*, BOOL)

	_textbox_MEMBERS;
};

/* The class typedef */
typedef struct _textbox *TextBox;

/* The constructor prototype */
struct _textbox *newTextBox(HINSTANCE instance, char *caption, int width, int height, int x, int y, enum _textboxtype multiline);

/* The destructor prototype */
void deleteTextBox(struct _textbox *textbox);



/* ======================================================Class Label============================================================================= */
/* Inheritance: sealed class, inherits from Control */
struct _label {
	struct _control base;

#define _label_MEMBERS \
	_control_MEMBERS \
	//TODO?

	_label_MEMBERS;
};

/* The class typedef */
typedef struct _label *Label;

/* The constructor prototype */
struct _label *newLabel(HINSTANCE instance, char *caption, int width, int height, int x, int y);

/* The destructor prototype */
void deleteLabel(struct _label *label);



/* ======================================================Class Pen================================================================================ */
/* Inheritance: sealed class, inherits from Object */
struct _pen {
	struct _object base;

#define _pen_MEMBERS \
	_object_MEMBERS; \
	/* fields */ \
	HPEN *handle; \
	int *penStyle; \
	int *width; \
	COLORREF *color; \
	/* methods */ \
	BOOL (*updateValue)(int, int, COLORREF); \
	BOOL (*updateValueT)(struct _pen*, int, int, COLORREF)

	_pen_MEMBERS;
};

/* The class typedef */
typedef struct _pen *Pen;

/* The constructor prototype */
struct _pen *newPen(int penStyle, int width, COLORREF color);

/* The destructor prototype */
void deletePen(struct _pen *pen);



/* ======================================================Class Brush============================================================================== */
/* Inheritance: sealed class, inherits from Object */
struct _brush {
	struct _object base;

#define _brush_MEMBERS \
	_object_MEMBERS; \
	/* fields */ \
	HBRUSH *handle; \
	UINT *brushStyle; \
	COLORREF *color; \
	ULONG_PTR *hatch; \
	/* methods */ \
	BOOL (*updateValue)(UINT, COLORREF, ULONG_PTR); \
	BOOL (*updateValueT)(struct _brush*, UINT, COLORREF, ULONG_PTR)

	_brush_MEMBERS;
};

/* The class typedef */
typedef struct _brush *Brush;

/* The constructor prototype */
struct _brush *newBrush(UINT brushStyle, COLORREF color, ULONG_PTR hatch);

/* The destructor prototype */
void deleteBrush(struct _brush *brush);







/* =============================================Callback typedefs=================================================================== */

typedef void(*Callback)(struct _guiobject*, void*, struct _eventargs*);
typedef void(*ButtonCallback)(struct _guiobject*, void*, struct _eventargs*);



/* =========================="STATIC" FUNCTION PROTOTYPES========================================================== */
static void *getCurrentThis();
void setCurrentThis(void *self);

BOOL displayWindow(struct _window *mainWindow, int nCmdShow);
HWND showControl(struct _control *control);


/* ======================================================Utility macros============================================================================= */
/* Safe initialization: returns NULL if malloc fails */
#define safeInit(field, type, value) if (!(field = (type*)malloc(sizeof(type)))) return NULL; *(field) = (type)value;

/* Anchor macros */
#define ANCHOR_LEFT 0xF000
#define ANCHOR_RIGHT 0x000F
#define ANCHOR_TOP 0x0F00
#define ANCHOR_BOTTOM 0x00F0

/* Color macros */
#define COLOR_RED RGB(0xFF, 0, 0)
#define COLOR_GREEN RGB(0, 0xFF, 0)
#define COLOR_BLUE RGB(0, 0, 0xFF)

/* "Method" call */
#define $(object) (setCurrentThis((void*)object), object)

/* Methods for adding children of types inherited from Control */
#define addButton(button) addChild((struct _guiobject*)button)
#define addTextBox(textbox) addChild((struct _guiobject*)textbox)
#define addLabel(label) addChild((struct _guiobject*)label)

/* Synchronize access to current object */
#define startSync(object) EnterCriticalSection(object->criticalSection)
#define endSync(object) LeaveCriticalSection(object->criticalSection)