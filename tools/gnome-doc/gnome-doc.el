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

;; insert header at current location
(defun gnome-doc-insert-header (function)
  (insert "/**\n * " function ":\n"))

;; insert a single variable, at current location
(defun gnome-doc-insert-var (var)
  (insert " * @" var ": \n"))

;; insert a 'blank' comment line
(defun gnome-doc-insert-blank ()
  (insert " * \n"))

;; insert a section comment line
(defun gnome-doc-insert-section (section)
  (insert " * " section ": \n"))

;; insert the end of the header
(defun gnome-doc-insert-footer ()
  (insert " **/\n"))

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
	  (if (re-search-forward "\\([A-Za-z_]+\\)[ \t\n]*\\(([^)]*)\\)" c-point nil)
	      (let ((c-argstart (match-beginning 2))
		    (c-argend (match-end 2)))
		(setq c-funcname (buffer-substring (match-beginning 1) (match-end 1)))
		(goto-char c-argstart)
		(while (re-search-forward "\\([A-Za-z_]*\\)[,)]" c-argend t)
		  (setq c-arglist
			(cons (buffer-substring (match-beginning 1) (match-end 1))
			      c-arglist))))))

	;; see if we already have a header here ...
	(save-excursion
	  (forward-line -1)
	  (while (looking-at "^ \\*")
	    (forward-line -1))
	  (if (looking-at "^/\\*\\*")
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
	      
	      (gnome-doc-insert-footer)))))
	
	;; goto the start of the description saved above
	(goto-char c-insert-here)))


;; set global binding for this key (follows the format for
;;   creating a changelog entry ...)
(global-set-key "\C-x4h"  'gnome-doc-insert)
