#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "XmHTMLP.h"
#include "XmHTMLfuncs.h"

/* scratch stuff */
static XmHTMLFormData *current_form;
static XmHTMLForm *current_entry;

static void
finalizeEntry(XmHTMLWidget html, XmHTMLForm *entry, Boolean insert) 
{
	if(entry->w)
	{
		GtkRequisition req;

		gtk_widget_size_request (entry->w, &req);
		entry->width = req.width;
		entry->height = req.height;
	}
	else
	{
		entry->width = 0;
		entry->height = 0;
	}

	/* add to parent form when requested */
	if(insert)
	{
		if(current_entry)
		{
			entry->prev = current_entry;
			current_entry->next = entry;
			current_entry = entry;
		}
		else
		{
			current_form->components = current_entry = entry;
		}
		/* and keep up component counter */
		current_form->ncomponents++;
	}
	_XmHTMLDebug(12, ("forms.c: finalizeEntry, added form entry, "
		"type = %i, name = %s\n", entry->type, entry->name));
}

/*****
* Name: 		getInputType
* Return Type: 	componentType
* Description: 	retrieves the type of an <input> HTML form member.
* In: 
*	attrib..:	attributes to check
* Returns:
*	componenttype if ``type'' is present in attributes. FORM_TEXT is
*	returned if type is not present or type is invalid/misspelled.
*****/
static componentType
getInputType(String attributes)
{
	String chPtr;
	componentType ret_val = FORM_TEXT;

	/* if type isn't specified we default to a textfield */
	if((chPtr = _XmHTMLTagGetValue(attributes, "type")) == NULL)
		return(ret_val);

	if(!(strcasecmp(chPtr, "text")))
		ret_val = FORM_TEXT;
	else if(!(strcasecmp(chPtr, "password")))
		ret_val = FORM_PASSWD;
	else if(!(strcasecmp(chPtr, "checkbox")))
		ret_val = FORM_CHECK;
	else if(!(strcasecmp(chPtr, "radio")))
		ret_val = FORM_RADIO;
	else if(!(strcasecmp(chPtr, "submit")))
		ret_val = FORM_SUBMIT;
	else if(!(strcasecmp(chPtr, "reset")))
		ret_val = FORM_RESET;
	else if(!(strcasecmp(chPtr, "file")))
		ret_val = FORM_FILE;
	else if(!(strcasecmp(chPtr, "hidden")))
		ret_val = FORM_HIDDEN;
	else if(!(strcasecmp(chPtr, "image")))
		ret_val = FORM_IMAGE;
	free(chPtr);
	return(ret_val);
}

void _XmHTMLFormReset(XmHTMLWidget html, XmHTMLForm *entry)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
}

/*****
* Name:			formCountComponents
* Return Type:	int
* Description:	count the number of client side components in a form 
*				(called from _XmHTMLFormActivate).
* In:
*   parent:		parent component of the the component that activated the
*   			callback.
*   comp:		component that activated the callback.
*   			
* Returns:
*	the number of client side components.
* Note:
*	written by: offer@sgi.com
*****/
static int
formCountComponents(XmHTMLForm *parent, XmHTMLForm *comp)
{
	int	count=1;
	
	current_entry = NULL;

	/* walk all components for this form and see which ones are selected */
	for(current_entry = parent; current_entry != NULL; 
		current_entry = current_entry->next)
	{
		switch((componentType)current_entry->type)
		{ 
			case FORM_SELECT:
				if(current_entry->multiple || current_entry->size > 1) 
				{
					/* list. Get count of all selected items */
					int *pos_list, pos_cnt = 0;

#if 0
					/* must take it from child, parent is a scrolledWindow */
					if((XmListGetSelectedPos(current_entry->child, &pos_list,
						&pos_cnt)))
					{
						count += pos_cnt;
						free(pos_list);	/* don't forget! */
					}
#else
					count++;
					fprintf (stderr, "FIXME: CountComponets: Missing code\n");
#endif
				}
				else
				{
					/* option menu, add entry when an item has been selected */
					XmHTMLForm *opt = NULL;
					for(opt = current_entry->options; opt != NULL;
						opt = opt->next)
					{
						if(opt->checked)
							count++;
					}
				}
				break;

			case FORM_CHECK:
			case FORM_RADIO:
				if(current_entry->checked) 
					count++;
				break;

			case FORM_IMAGE:
				if(comp == current_entry) 
					count+=2; 	/* name.x=... and name.y=... */
				break;

			case FORM_RESET:
			case FORM_SUBMIT:
				if(comp == current_entry) 
					count++; 

			case FORM_PASSWD:
				if(current_entry->content != NULL)
					count++;
				break;

			/* only return text fields if these actually contain text */
			case FORM_TEXT:
				/* FIXME: check forms.c: check for text contents here */
				count++;
				break;
			case FORM_FILE:
				/* FIXME: check forms.c: check for text contents here */
				count++;
				break;
			case FORM_TEXTAREA:
				count++;
				/* FIXME: check forms.c: check for text contents here */
				break;

			/* hidden fiels are always returned */
			case FORM_HIDDEN:
				count++;
				break;

			case FORM_OPTION:
				/* is a wrapper, so doesn't do anything */
				break;
			/* no default */
		}
	}
	return(count);
}

void
_XmHTMLFormActivate(XmHTMLWidget html, TEvent *event, XmHTMLForm *entry)
{
	XmHTMLFormCallbackStruct cbs;
	XmHTMLFormDataPtr components;
	int nComponents;
	int	i, j;
	String chPtr;

	_XmHTMLDebug(12, ("forms.c: _XmHTMLFormActivate, activated by component "
		"%s\n", entry->name));

	/* only do something when a form callback has been installed */
	if(CHECK_CALLBACK (html, form_callback, FORM) == NULL)
		return;

	/*****
	* Check which components of the current form should be returned.
	*
	* Seems time consuming stepping through the link list twice, but this way 
	* we can guarantee that we malloc the right ammount of memory (there isn't 
	* a one-to-one mapping for internal and application views of the
	* components, _and_ we won't frag memory unlike repeated calls to realloc 
	* -- rmo 
	*****/	
	nComponents = formCountComponents(entry->parent->components, entry);
	components = (XmHTMLFormDataPtr)calloc(nComponents,
					sizeof(XmHTMLFormDataRec)); 
	
	current_entry = NULL;
	for(current_entry = entry->parent->components, j=0;
		current_entry != NULL && j < nComponents; 
		current_entry = current_entry->next)
	{
		/* default settings for this entry. Overridden when required below */
		components[j].type  = current_entry->type;
		components[j].name  = current_entry->name;

		switch((componentType)current_entry->type)
		{ 
			case FORM_SELECT:
				/*****
				* Option menu, get value of selected item (size check required
				* as multiple is false for list boxes offering a single
				* entry).
				*****/
				if(!current_entry->multiple && current_entry->size == 1)
				{ 
					XmHTMLForm *opt = NULL;

					/*****
					* Get selected item (if any). Only one item can be
					* selected at a time as this is an option menu.
					*****/
					for(opt = current_entry->options; opt != NULL &&
						!opt->checked; opt = opt->next);

					if(opt)
					{
						components[j].name  = current_entry->name;
						components[j].type  = FORM_OPTION;	/* override */
						components[j].value = opt->value; 
						j++;
					}
				}
				else
				{
					/* list. Get all selected items and store them */
					int *pos_list, pos_cnt = 0;

					fprintf (stderr, "FormActivate: Missing chunk of code #1\n");
				}
				break;

			/* password entry has really entered text stored */
			case FORM_PASSWD:
				if(current_entry->content != NULL)
					components[j++].value = current_entry->content;
				break;

			/* textfield contents aren't stored by us */
			case FORM_TEXT:
				chPtr = gtk_entry_get_text (GTK_ENTRY (current_entry->w));
				components[j++].value = chPtr;
				break;

			/*****
			* File contents aren't stored by us and must be taken from the
			* textfield child.
			*****/
			case FORM_FILE:
				chPtr = gtk_entry_get_text (GTK_ENTRY (current_entry->child));
				components[j++].value = chPtr;
				break;
				
			/*****
			* Textarea contents aren't stored by us and must be taken from
			* the child (current_entry->w is the id of the scrolled window
			* parent for this textarea)
			*****/
			case FORM_TEXTAREA:
				chPtr = gtk_entry_get_text (GTK_ENTRY (current_entry->child));
				components[j++].value = chPtr;
				break;
				
			/* check/radio boxes are equal in here */
			case FORM_CHECK:
			case FORM_RADIO:
				if(current_entry->checked)
					components[j++].value = current_entry->value;
				break;

			case FORM_IMAGE:
				if(entry == current_entry)
				{ 
					char *xname, *yname;
					char *x, *y;
					xname = calloc(strlen(current_entry->name)+3, sizeof(char));
					yname = calloc(strlen(current_entry->name)+3, sizeof(char));
					x= calloc(16, sizeof(char));
					y= calloc(16, sizeof(char));
					
					memcpy(xname, current_entry->name,
						strlen(current_entry->name)); 
					memcpy(yname, current_entry->name,
						strlen(current_entry->name)); 
					strcat(xname,".x");
					strcat(yname,".y");
#if 0
					fprintf (stderr, "FIXME: ButtonXY positionsc should be computed\n");
					sprintf(x,"%d", event->xbutton.x - entry->data->x); 
					sprintf(y,"%d", event->xbutton.y - entry->data->y);
#endif
					components[j].name  = xname;	/* override */
					components[j].value = x;
					j++;
					components[j].name  = yname;	/* override */
					components[j].value = y;
					j++;
				}
				break;

			/* always return these */
			case FORM_HIDDEN:
				components[j++].value = current_entry->value;
				break;

			/* reset and submit are equal in here */
			case FORM_RESET:
			case FORM_SUBMIT:
				if(entry == current_entry)
					components[j++].value = current_entry->value;
				break;

			case FORM_OPTION:
				/* is a wrapper, so doesn't do anything */
				break;
			/* no default */
		}
	}	
	(void)memset(&cbs, 0, sizeof(XmHTMLFormCallbackStruct));

	cbs.reason      = XmCR_HTML_FORM;
	cbs.event       = event;
	cbs.action      = strdup(entry->parent->action);
	cbs.method      = entry->parent->method;
	cbs.enctype     = strdup(entry->parent->enctype);
	cbs.ncomponents = nComponents;
	cbs.components  = components;

	Toolkit_Call_Callback((Widget)html, html->html.form_callback, FORM, &cbs);

	/* free all */
	for(i = 0; i < j; i++)
	{ 
		/* value of these components is retrieved using XmTextGetValue */
		if(components[i].type == FORM_TEXT || components[i].type == FORM_FILE ||
			components[i].type == FORM_TEXTAREA )
			if(components[i].value) 
				g_free (components[i].value);
		/* use free to avoid FMM errors in purify */
		if(components[i].type == FORM_IMAGE)
		{
			if(components[i].value) 
				free(components[i].value);
			if(components[i].name) 
				free(components[i].name);
		}
	}
	free(components);
	free(cbs.action);
	free(cbs.enctype);
}

void 
_XmHTMLFreeForm(XmHTMLWidget html, XmHTMLFormData *form)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
}

XmHTMLForm*
_XmHTMLFormAddTextArea(XmHTMLWidget html, String attributes, String text)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
	return NULL;
}

void
_XmHTMLFormSelectClose(XmHTMLWidget html, XmHTMLForm *entry)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
	printf ("forms: Select Close\n");
}

void
_XmHTMLFormSelectAddOption(XmHTMLWidget html, XmHTMLForm *entry,
	String attributes, String label)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
}

XmHTMLForm*
_XmHTMLFormAddSelect(XmHTMLWidget html, String attributes)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
	return 0;
}

static void
checkbox_changed (GtkWidget *w, void *data)
{
	XmHTMLForm *entry = (XmHTMLForm*)data;

	entry->checked = !entry->checked;
}

static void
radio_changed (GtkWidget *w, void *data)
{
	XmHTMLForm *tmp, *entry = (XmHTMLForm*)data;

	entry->checked = !entry->checked;

	/* toggle set, unset all other toggles */
	if(entry->checked)
	{
		/* get start of this radiobox */
		for(tmp = entry->parent->components; tmp != NULL; tmp = tmp->next)
			if(tmp->type == FORM_RADIO && !(strcasecmp(tmp->name, entry->name)))
				break;

		/* sanity */
		if(tmp == NULL)
			return;

		/* unset all other toggle buttons in this radiobox */
		for(; tmp != NULL; tmp = tmp->next)
		{
			if(tmp->type == FORM_RADIO && tmp != entry)
			{
				/* same group, unset it */
				if(!(strcasecmp(tmp->name, entry->name)))
				{
					gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (tmp->w), 0);
					tmp->checked = False;
				}
				/*****
				* Not a member of this group, we processed all elements in
				* this radio box, break out.
				*****/
				else
					break;
			}
		}
	}
	else /* current toggle can't be unset */
	{
		gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (entry->w), 1);
		entry->checked = True;
	}
}

static void
button_clicked (GtkWidget *widget, XmHTMLForm *entry)
{
	XmHTMLWidget html;
	
	html = GTK_XMHTML (entry->parent->html);

	if (entry->type == FORM_SUBMIT){
		_XmHTMLDebug(12, ("forms.c: buttonActivateCB for FORM_SUBMIT\n"));
		_XmHTMLFormActivate(html, NULL, entry);
	} else if (entry->type == FORM_RESET){
		_XmHTMLDebug(12, ("forms.c: buttonActivateCB for FORM_RESET\n"));
		_XmHTMLFormReset(html, entry);
	}
	
}

XmHTMLForm*
_XmHTMLFormAddInput(XmHTMLWidget html, String attributes)
{
	static XmHTMLForm *entry;
	char   *chPtr;

	if(attributes == NULL)
		return(NULL);

	if(current_form == NULL)
	{
		_XmHTMLWarning(__WFUNC__(html, "_XmHTMLFormAddInput"),
			"Bad HTML form: <INPUT> not within form.");
	}

	/* Create and initialise a new entry */
	entry = (XmHTMLForm*)malloc(sizeof(XmHTMLForm));
	(void)memset(entry, 0, sizeof(XmHTMLForm));

	/* set parent form */
	entry->parent = current_form;

	entry->type = getInputType(attributes);

	/* get name */
	if((entry->name = _XmHTMLTagGetValue(attributes, "name")) == NULL)
	{
		switch(entry->type)
		{
			case FORM_TEXT:
				chPtr = "text";
				break;
			case FORM_PASSWD:
				chPtr = "Password";
				break;
			case FORM_CHECK:
				chPtr = "CheckBox";
				break;
			case FORM_RADIO:
				chPtr = "RadioBox";
				break;
			case FORM_RESET:
				chPtr = "Reset";
				break;
			case FORM_FILE:
				chPtr = "File";
				break;
			case FORM_IMAGE:
				chPtr = "Image";
				break;
			case FORM_HIDDEN:
				chPtr = "Hidden";
				break;
			case FORM_SUBMIT:
				chPtr = "Submit";
				break;
		}
		entry->name = strdup(chPtr);
	}

	entry->value = _XmHTMLTagGetValue(attributes, "value");
	entry->checked = _XmHTMLTagCheck(attributes, "checked");
	entry->selected = entry->checked;	/* save default state */

	if(entry->type == FORM_TEXT || entry->type == FORM_PASSWD)
	{
		/* default to 25 columns if size hasn't been specified */
		entry->size = _XmHTMLTagGetNumber(attributes, "size", 25);

		/* unlimited amount of text input if not specified */
		entry->maxlength = _XmHTMLTagGetNumber(attributes, "maxlength", -1);

		/* passwd can't have a default value */
		if(entry->type == FORM_PASSWD && entry->value)
		{
			free(entry->value);
			entry->value = NULL;
		}
		/* empty value if none given */
		if(entry->value == NULL)
		{
			entry->value = (String)malloc(1);
			entry->value[0] = '\0';
		}
	}
	else if(entry->type == FORM_FILE)
	{
		/* default to 20 columns if size hasn't been specified */
		entry->size = _XmHTMLTagGetNumber(attributes, "size", 20);

		/* check is we are to support multiple selections */
		entry->multiple = _XmHTMLTagCheck(attributes, "multiple");

		/* any dirmask to use? */
		entry->value   = _XmHTMLTagGetValue(attributes, "value");
		entry->content = _XmHTMLTagGetValue(attributes, "src");
	}
	entry->align = _XmHTMLGetImageAlignment(attributes);

	/*****
	* go create the actual widget
	* As image buttons are promoted to image words we don't deal with the
	* FORM_IMAGE case. For hidden form fields nothing needs to be done.
	*****/
	if(entry->type != FORM_IMAGE && entry->type != FORM_HIDDEN)
	{
		switch(entry->type)
		{
			/* text field, set args and create it */
			case FORM_TEXT:
			case FORM_PASSWD:
				entry->w = gtk_entry_new_with_max_length (entry->size);
				gtk_entry_set_text (GTK_ENTRY (entry->w), entry->value);
				/* FIXME:
				 *    set columns,
				 *    set maxlenght
				 *    handle passwords
				 */
				break;

			/* toggle buttons, set args and create */
			case FORM_CHECK:
				entry->w = gtk_check_button_new_with_label ("");
				gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (entry->w), entry->checked);
				gtk_signal_connect (GTK_OBJECT (entry->w), "toggled", (GtkSignalFunc)
						    checkbox_changed, entry);
				break;
				
			case FORM_RADIO:
				entry->w = gtk_radio_button_new_with_label (NULL, "");
				gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (entry->w), entry->checked);
				gtk_signal_connect (GTK_OBJECT (entry->w), "toggled", (GtkSignalFunc)
						    radio_changed, entry);
				break;

			/*****
			* special case: this type of input is a textfield with a ``browse''
			* button.
			*****/
			case FORM_FILE:
				fprintf (stderr, "FORM_FILE not handled\n");
				break;
				
			case FORM_RESET:
			case FORM_SUBMIT:
				entry->w = gtk_button_new_with_label (entry->name);
				gtk_signal_connect (GTK_OBJECT(entry->w), "clicked",
						    (GtkSignalFunc) button_clicked, entry);
				break;
			default:
				break;
		}
	}
#if 0
		/* defaults for all widgets */
		argc = 0;
		XtSetArg(args[argc], XmNmappedWhenManaged, False); argc++;
		XtSetArg(args[argc], XmNborderWidth, 0); argc++;

		/*****
		* Check if we may use the document colors for the form components.
		*****/
		if(html->html.allow_form_coloring)
		{
			XtSetArg(args[argc], XmNbackground, html->html.body_bg); argc++;
			XtSetArg(args[argc], XmNforeground, html->html.body_fg); argc++;
		}
		XtSetArg(args[argc], XmNfontList, my_fontList); argc++;
#endif
	/* manage it */
	if(entry->w)
		gtk_container_add (GTK_CONTAINER (html), entry->w);

	/* do final stuff for this entry */
	finalizeEntry(html, entry, True);

	/* all done */
	return(entry);
}

void
_XmHTMLEndForm(XmHTMLWidget html)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
}

void
_XmHTMLStartForm(XmHTMLWidget html, String attributes)
{
	static XmHTMLFormData *form;

	/* empty form, no warning, just return */
	if(attributes == NULL)
		return;

	/* allocate a new entry */
	form = (XmHTMLFormData*)malloc(sizeof(XmHTMLFormData));
	/* initialise to zero */
	memset(form, 0, sizeof(XmHTMLFormData));

	/* this form starts a new set of entries */
	current_entry = NULL;

	/* set form owner */
	form->html = html;

	/* pick up action */
	if((form->action = _XmHTMLTagGetValue(attributes, "action")) == NULL)
	{
		/* the action tag is required, so destroy and return if not found */
		free(form);
		form = NULL;
#ifdef PEDANTIC
		_XmHTMLWarning(__WFUNC__(html, "_XmHTMLStartForm"),
			"Bad HTML form: no action tag found, form ignored.");
#endif
		return;
	}
	/* default method is get */
	form->method = XmHTML_FORM_GET;
	{
		char *method = _XmHTMLTagGetValue(attributes, "method"); 
		if(method != NULL)
		{ 
			if(!strncasecmp(method, "get", 3))
				form->method = (int)XmHTML_FORM_GET;
			else if(!strncasecmp(method, "post", 4))
				form->method = (int)XmHTML_FORM_POST;
			else if(!strncasecmp(method, "pipe", 4))
				form->method = (int)XmHTML_FORM_PIPE;
			free(method);
		}
	}

	/* form encoding */
	if((form->enctype = _XmHTMLTagGetValue(attributes, "enctype")) == NULL)
		form->enctype = strdup("application/x-www-form-urlencoded");

	if(html->html.form_data)
	{
		form->prev = current_form;
		current_form->next = form;
		current_form = form;
	}
	else
		html->html.form_data = current_form = form;
	_XmHTMLDebug(12, ("forms.c: _XmHTMLStartForm, created a new form "
		"entry, action = %s\n", form->action));

}
