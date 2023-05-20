int
c_check_intsuffix(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case 'u':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_USUFFIX;
		break;
	case 'i':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		break;
	case 'l':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_LSUFFIX;
		break;
	default:
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		return 1;
	}
	return 0;
}

int
c_state_number_errsuffix(struct c_state *s, struct c_info *i)
{
	if(isalnum(i->data[i->index])) {
		s->attr = A_REVERSE | COLOR_PAIR(2);
		return 0;
	}
	c_popstate(s);
	return 1;
}

int
c_state_number_usuffix(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case 'l':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_LSUFFIX;
		break;
	default:
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		return 1;	
	}
	return 0;
}

int
c_state_number_lsuffix(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case 'l':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		break;
	default:
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		return 1;	
	}
	return 0;
}

int
c_state_number_zero(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case 'x': case 'X':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_HEX;
		break;
	case '0' ... '7':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_OCTAL;
		break;
	case 'B': case 'b':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_BINARY;
		break;
	case '.':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_FLOAT;
		break;
	case 'e':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_NEGEXP;
		break;
	default:
		return c_check_intsuffix(s, i);
	}
	return 0;
}

int
c_state_number_decimal(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case '0' ... '9':
		s->attr = COLOR_PAIR(7);
		break;
	case '.':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_FLOAT;
		break;
	case 'e':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_NEGEXP;
		break;
	default:
		return c_check_intsuffix(s, i);
	}
	return 0;
}

int
c_state_number_hex(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case 'a' ... 'f':
	case 'A' ... 'F':
	case '0' ... '9':
		s->attr = COLOR_PAIR(7);
		break;
	default:
		return c_check_intsuffix(s, i);
	}
	return 0;
}

int
c_state_number_octal(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case '0' ... '7':
		s->attr = COLOR_PAIR(7);
		break;
	default:
		return c_check_intsuffix(s, i);
	}
	return 0;
}

int
c_state_number_binary(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case '0':
	case '1':
		s->attr = COLOR_PAIR(7);
		break;
	default:
		return c_check_intsuffix(s, i);
	}
	return 0;
}

int
c_state_number_float(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case '0' ... '9':
		s->attr = COLOR_PAIR(7);
		break;
	case 'e':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_NEGEXP;
		break;
	case 'f': case 'F':
	case 'l': case 'L':
	case 'd': case 'D':
		s->attr = COLOR_PAIR(7);
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		break;
	default:
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		return 1;
	}
	return 0;
}

int
c_state_number_maybefloat(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case '0' ... '9':
		s->state = C_STATE_NUMBER_FLOAT;
		s->attr = COLOR_PAIR(7);
		break;
	default:
		s->attr = COLOR_PAIR(4);
		c_popstate(s);
	}
	return 0;
}

int
c_state_number_exp(struct c_state *s, struct c_info *i)
{
	switch(i->data[i->index]) {
	case '0' ... '9':
		s->attr = COLOR_PAIR(7);
		break;
	case 'f': case 'F':
	case 'l': case 'L':
	case 'd': case 'D':
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		s->attr = COLOR_PAIR(7);
		break;
	default:
		s->state = C_STATE_NUMBER_ERRSUFFIX;
		return 1;
	}
	return 0;
}

int
c_state_number_negexp(struct c_state *s, struct c_info *i)
{
	s->state = C_STATE_NUMBER_EXP;
	switch(i->data[i->index]) {
	case '-':
		s->attr = COLOR_PAIR(7);
		break;
	default:
		return 1;
	}
	return 0;
}
