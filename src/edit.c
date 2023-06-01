#include "cnergy.h"

int (**edit_statesfromfiletype(const char *file))(struct state *s)
{
	struct {
		const char *fileExt;
		int (**states)(struct state *s);
	} sfft[] = {
		{ "c", c_states },
		{ "h", c_states },
		{ "c++", c_states },
		{ "cpp", c_states },
		{ "cxx", c_states },
		{ "hpp", c_states },
		{ "cng", cnergy_states },
		{ "cnergy", cnergy_states },
	};
	const char *ext;
	// just check file ending for now, maybe also use libmagic further down the road
	ext = strrchr(file, '.');
	if(!ext)
		return NULL;
	ext++;
	for(unsigned i = 0; i < ARRLEN(sfft); i++)
		if(!strcmp(ext, sfft[i].fileExt))
			return sfft[i].states;
	return NULL;	
}

struct window *
edit_new(struct buffer *buf, int (**states)(struct state *s))
{
	struct window *win;

	win = window_new(WINDOW_EDIT);
	if(!win)
		return NULL;
	win->buffer = buf;
	win->states = states;
	return win;
}

int
edit_type(struct window *win, const char *str, size_t nStr)
{
	buffer_insert(win->buffer, str, nStr);
	return 0;
}

int
edit_render(struct window *win)
{
	struct buffer *buf;
	int curLine, curCol;
	size_t saveiGap;
	int nLineNumbers;
	struct state state;
	size_t minSel, maxSel;

	// get buffer and cursor position
	buf = win->buffer;
	curLine = buffer_line(buf);
	curCol = buffer_col(buf);
	// move gap out of the way
	saveiGap = buf->iGap;
	unsafe_buffer_movecursor(buf, buf->nData - buf->iGap);
	// make sure there is an EOF at the end of the data (this is for convenience)
	if(!buf->nGap) {
		char *const newData = dialog_realloc(buf->data, buf->nData + BUFFER_GAP_SIZE);
		if(!newData)
			return -1;
		buf->data = newData;
		buf->nGap = BUFFER_GAP_SIZE;
	}
	buf->data[buf->nData] = EOF;

	// prepare state
	memset(&state, 0, sizeof(state));

	// scroll the text if the caret is out of view
	if(curLine < win->vScroll)
		win->vScroll = curLine;
	if(curLine >= win->lines - 1 + win->vScroll)
		win->vScroll = curLine - (win->lines - 2);

	state.minLine = win->vScroll;
	state.maxLine = win->vScroll + win->lines - 1;

	// get width of line numbers
	nLineNumbers = 1; // initial padding to the right side
	if(window_left(win))
		nLineNumbers++; // add some padding
	for(int i = state.maxLine; i; i /= 10)
		nLineNumbers++;

	// note that we scroll a little extra horizontally
	// TODO: fix bug where scrolling will sometimes cause the cursor to be misaligned with a multi width character
	if(curCol <= win->hScroll)
		win->hScroll = MAX((int) curCol - win->cols / 4, 0);
	if(curCol >= win->cols - nLineNumbers + win->hScroll)
		win->hScroll = curCol - (win->cols - nLineNumbers - 2) + win->cols / 4;

	state.minCol = win->hScroll;
	state.maxCol = win->hScroll - nLineNumbers + win->cols;

	// get bounds of selection
	if(win == focus_window && (win->bindMode->flags & FBIND_MODE_SELECTION)) {
		if(win->selection > saveiGap) {
			minSel = saveiGap;
			maxSel = win->selection;
		} else {
			minSel = win->selection;
			maxSel = saveiGap;
		}
	} else {
		// no selection
		minSel = 1;
		maxSel = 0;
	}

	// setup loop
	move(win->line, win->col);
	// draw first line if needed
	if(!win->vScroll) {
		attrset(win == focus_window ? COLOR(11, 8) : COLOR(3, 8));
		printw("%*d ", nLineNumbers - 1, 1);
	}
	state.win = win;
	state.data = buf->data;
	state.nData = buf->nData;
	state.cursor = saveiGap;
	state.line = 0;
	state.col = 0;

	// draw all data
	for(size_t i = 0; i < buf->nData; state.index = i) {
		if(win->states)
			state_continue(&state);
		if(state.conceal) {
			const char *conceal;

			conceal = state.conceal;
			state.conceal = NULL;
			if(state.line != curLine) {
				i = state.index + 1;
				if(state.line >= state.minLine && state.line < state.maxLine) {
					attrset(state.attr);
					while(*conceal) {
						const size_t len = utf8_len(conceal, strlen(conceal));
						if(state.col >= state.minCol && state.col < state.maxCol)
							addnstr(conceal, len);
						conceal += len;
						state.col++;
					}
				}
				continue;
			}
		}
		// check if the state interpreted the utf8 encoded character correctly (if not we handle it here)
		if((buf->data[i] & 0x80) && state.index == i) {
			attrset(state.attr);
			const size_t len = utf8_len(buf->data + i, buf->nData - i);
			if(state.col >= state.minCol && state.col < state.maxCol) {
				if(i >= minSel && i <= maxSel)
					attron(A_REVERSE);
				if(!utf8_valid(buf->data + i, buf->nData - i)) {
					attron(COLOR(4, 0));
					if(state.col >= state.minCol && state.col < state.maxCol)
						addch('^');
					if(state.col + 1 >= state.minCol && state.col + 1 < state.maxCol)
						addch('?');
				} else {
					addnstr(buf->data + i, len);
				}
			}
			state.col += utf8_width(buf->data + i, buf->nData - i, state.col);
			i += len;
			continue;
		}
		for(; i <= state.index; i++) {
			const char ch = buf->data[i];
			attrset(state.attr);
			if(i >= minSel && i <= maxSel)
				attron(A_REVERSE);
			if(ch == '\t') {
				do {
					if(state.line >= state.minLine && state.line < state.maxLine &&
							state.col >= state.minCol && state.col < state.maxCol)
						addch(' ');
				} while((++state.col) % TABSIZE);
			} else if(ch == '\n') {
				if(state.line >= state.minLine && state.line < state.maxLine) {
					// draw extra space for selection
					if(i >= minSel && i <= maxSel) {
						if(state.col >= state.minCol && state.col < state.maxCol) {
							attrset(A_REVERSE);
							addch(' ');
						}
						state.col++;
					}
					// erase rest of line
					attrset(0);
					state.col = MAX(state.col, state.minCol);
					if(state.col < state.maxCol)
						printw("%*s", state.maxCol - state.col, "");
				}
				state.line++;
				if(state.line >= state.minLine && state.line < state.maxLine) {
					// draw line prefix
					attrset(win == focus_window ? COLOR(11, 8) : COLOR(3, 8));
					mvprintw(win->line + state.line - state.minLine, win->col, "%*d ", nLineNumbers - 1, state.line + 1);
				}
				state.col = 0;
			} else if(iscntrl(ch)) {
				if(state.line >= state.minLine && state.line < state.maxLine) {
					attron(COLOR(4, 0));
					if(state.col >= state.minCol && state.col < state.maxCol)
						addch('^');
					if(state.col + 1 >= state.minCol && state.col + 1 < state.maxCol)
						addch(ch == 0x7f ? '?' : ch + 'A' - 1);
				}
				state.col += 2;
			} else {
				if(state.line >= state.minLine && state.line < state.maxLine && state.col >= state.minCol && state.col < state.maxCol)
					addch(ch);
				state.col++;
			}
		}
	}
	// erase rest of line and draw little waves until we hit the status bar
	attrset(COLOR(6, 0));
	while(state.line < state.maxLine) {
		state.col = MAX(state.col, state.minCol);
		if(state.col < state.maxCol)
			printw("%*s", state.maxCol - state.col, "");
		state.line++;
		if(state.line >= state.maxLine)
			break;
		mvprintw(state.line + win->line - state.minLine, win->col, "%*s~ ", nLineNumbers - 2, "");
		state.col = 0;
	}
	state_cleanup(&state);
	// draw status bar (TODO: what if the status bar doesn't fit?)
	attrset(win == focus_window ? COLOR(14, 8) : COLOR(6, 8));
	mvaddstr(win->line + win->lines - 1, win->col, " Buffer");
	attrset(win == focus_window ? COLOR(11, 8) : COLOR(3, 8));
	if(buf->saveEvent != buf->iEvent)
		addch('*');
	if(buf->file)
		printw(" %s", buf->file);
	printw(" %u, %u", curLine, curCol);
	printw("%*s", MAX(win->col + win->cols - getcurx(stdscr), 0), "");
	// set the global end caret position
	if(win == focus_window) {
		focus_y = win->line + curLine - state.minLine;
		focus_x = win->col + curCol + nLineNumbers - state.minCol;
	}
	// move back the gap before we leave
	unsafe_buffer_movecursor(buf, saveiGap - buf->iGap);
	return 0;
}

bool
edit_bindcall(struct window *win, struct binding_call *bc, ssize_t param)
{
	struct buffer *const buf = win->buffer;
	switch(bc->type) {
	case BIND_CALL_SETMODE:
		if(win->bindMode->flags & FBIND_MODE_SELECTION)
			win->selection = buf->iGap;
		return true;
	case BIND_CALL_ASSERT:
		return !memcmp(bc->str, buf->data + buf->iGap + buf->nGap, MIN(strlen(bc->str), buf->nData - buf->iGap));
	case BIND_CALL_MOVECURSOR: return buffer_movecursor(buf, param) == param;
	case BIND_CALL_MOVEHORZ: return buffer_movehorz(buf, param) == param;
	case BIND_CALL_MOVEVERT: return buffer_movevert(buf, param) == param;
	case BIND_CALL_INSERT:
		buffer_insert(buf, bc->str, strlen(bc->str));
		return true;
	case BIND_CALL_DELETE: return buffer_delete(buf, param) == param;
	case BIND_CALL_DELETELINE: return buffer_deleteline(buf, param) == param;
	case BIND_CALL_DELETESELECTION: {
		size_t n;

		if(!(win->bindMode->flags & FBIND_MODE_SELECTION) || !buf->nData)
			return false;
		if(win->selection > buf->iGap)
			n = win->selection - buf->iGap + 1;
		else {
			n = buf->iGap - win->selection + 1;
			buffer_movecursor(buf, win->selection - buf->iGap);
		}
		buffer_delete(buf, n);
		win->selection = buf->iGap;
		return true;
	}
	case BIND_CALL_COPY: {
		char *text;
		size_t nText;

		if(!(win->bindMode->flags & FBIND_MODE_SELECTION) || !buf->nData)
			break;
		if(win->selection > buf->iGap) {
			text = buf->data + buf->iGap + buf->nGap;
			nText = win->selection - buf->iGap + 1;
		} else {
			text = buf->data + win->selection;
			nText = buf->iGap - win->selection + 1;
		}
		return !clipboard_copy(text, nText);
	}
	case BIND_CALL_PASTE: {
		char *text;

		if(!clipboard_paste(&text)) {
			buffer_insert(buf, text, strlen(text));
			free(text);
			return true;
		}
		return false;
	}
	case BIND_CALL_UNDO:
		return buffer_undo(buf);
	case BIND_CALL_REDO:
		return buffer_redo(buf);
	case BIND_CALL_WRITEFILE:
		return buffer_save(win->buffer);
	default:
		return false;
	}
	return true;
}
