;;
;;  (c) 1998 Michael Zucchi, All Rights Reserved
;;
;;  'gnome-doc' helper function for gnome comments.
;;
;;  Load into emacs and use with C-x4h, to insert a new header for
;;  the current function.
;;
;;  This program is free software; you can redistribute it and/or
;;  modify it under the terms of the GNU General Public License
;;  as published by the Free Software Foundation; either version 2 of
;;  the License, or (at your option) any later version.
;;
;;  This program is distributed in the hope that it will be useful,
;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;  GNU General Public License for more details.
;;
;;  You should have received a copy of the GNU General Public
;;  License along with this program; if not, write to the Free Software
;;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

;;;;
;; This is my first attempt at anything remotely lisp-like, you'll just
;; have to live with it :)
;;
;; It works ok with emacs-20, AFAIK it should work on other versions too.

(defgroup gnome-doc nil 
"Generates automatic headers for functions"
:prefix "gnome"
:group 'tools)

(defcustom gnome-doc-header "/**\n * %s:\n"
"Start of documentation header.

Using '%s' will expand to the name of the function."
:type '(string)
:group 'gnome-doc)

(defcustom gnome-doc-blank " * \n"
"Used to put a blank line into the header."
:type '(string)
:group 'gnome-doc)

(defcustom gnome-doc-trailer " **/\n"
"End of documentation header.

Using '%s' will expand to the name of the function being defined."
:type '(string)
:group 'gnome-doc)

(defcustom gnome-doc-parameter " * @%s: \n"
"Used to add another parameter to the header.

Using '%s' will be replaced with the name of the parameter."
:type '(string)
:group 'gnome-doc)

(defcustom gnome-doc-section " * %s: \n"
"How to define a new section in the output.

Using '%s' is replaced with the name of the section.
Currently this will only be used to define the 'return value' field."
:type '(string)
:group 'gnome-doc)

(defcustom gnome-doc-match-block "^ \\*"
"This is a regular expression which will be used to match lines in a header.

It should match every line produced by any of the header specifiers.
It is therefore convenient to start all header lines with a common
comment prefix"
:type '(string)
:group 'gnome-doc)

(defcustom gnome-doc-match-header "^/\\*\\*"
"This is a regular expression which will be used to match the first line of a header.

It is used to identify if a header has already been applied to this
function.  It should match the line produced by gnome-doc-header, or
at least the first line of this which matches gnome-doc-match-block"
:type '(string)
:group 'gnome-doc)


(make-variable-buffer-local 'gnome-doc-header)
(make-variable-buffer-local 'gnome-doc-trailer)
(make-variable-buffer-local 'gnome-doc-parameter)
(make-variable-buffer-local 'gnome-doc-section)
(make-variable-buffer-local 'gnome-doc-blank)
(make-variable-buffer-local 'gnome-doc-match-block)
(make-variable-buffer-local 'gnome-doc-match-header)

;; insert header at current location
(defun gnome-doc-insert-header (function)
  (insert (format gnome-doc-header function)))

;; insert a single variable, at current location
(defun gnome-doc-insert-var (var)
  (insert (format gnome-doc-parameter var)))

;; insert a 'blank' comment line
(defun gnome-doc-insert-blank ()
  (insert gnome-doc-blank))

;; insert a section comment line
(defun gnome-doc-insert-section (section)
  (insert (format gnome-doc-section section)))

;; insert the end of the header
(defun gnome-doc-insert-footer (func)
  (insert (format gnome-doc-trailer func)))

(defun gnome-doc-insert ()
  "Add a gnome documentation header to the current function.  Only C
function types are properly supported at the moment."
  (interactive)
  (let (c-insert-here (point))
    (save-excursion
      (beginning-of-defun)
      (let (c-arglist
	    c-funcname
	    (c-point (point))
	    c-comment-point
	    c-isvoid
	    c-doinsert)
	(search-backward "(")
	(forward-line -2)
	(while (or (looking-at "^$")
		   (looking-at "^ \\*")
		   (looking-at "^#"))
	  (forward-line 1))
	(if (or (looking-at ".*void.*(")
		(looking-at ".*void[ \t]*$"))
	    (setq c-isvoid 1))
	(save-excursion
	  (if (re-search-forward "\\([A-Za-z_:~]+\\)[ \t\n]*\\(([^)]*)\\)" c-point nil)
	      (let ((c-argstart (match-beginning 2))
		    (c-argend (match-end 2)))
		(setq c-funcname (buffer-substring (match-beginning 1) (match-end 1)))
		(goto-char c-argstart)
		(while (re-search-forward "\\([A-Za-z_]*\\)[,)]" c-argend t)
		  (setq c-arglist
			(append c-arglist
				(list (buffer-substring (match-beginning 1) (match-end 1)))))))))

	;; see if we already have a header here ...
	(save-excursion
	  (forward-line -1)
	  (while (looking-at gnome-doc-match-block)
	    (forward-line -1))
	  (if (looking-at gnome-doc-match-header)
	      (error "Header already exists")
	    (setq c-doinsert t)))

	;; insert header
	(if c-doinsert
	    (progn
	      (gnome-doc-insert-header c-funcname)
	
	      ;; all arguments
	      (while c-arglist
		(gnome-doc-insert-var (car c-arglist))
		(setq c-arglist (cdr c-arglist)))
	
	      ;; finish it off
	      (gnome-doc-insert-blank)
	      (gnome-doc-insert-blank)
	      ;; record the point of insertion
	      (setq c-insert-here (- (point) 1))

	      ;; only insert a returnvalue if we have one ...
	      (if (not c-isvoid)
		  (progn
		    (gnome-doc-insert-blank)
		    (gnome-doc-insert-section "Return Value")))
	      
	      (gnome-doc-insert-footer c-funcname)))))
	
	;; goto the start of the description saved above
	(goto-char c-insert-here)))


;; set global binding for this key (follows the format for
;;   creating a changelog entry ...)
(global-set-key "\C-x4h"  'gnome-doc-insert)
