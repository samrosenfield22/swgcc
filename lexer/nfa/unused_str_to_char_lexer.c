lextok *chars_to_substrings_lexer(const char *instr)
{
		lextok *l = calloc(strlen(instr)+1, sizeof(*l));
		assert(l);
		lextok *lp = l;

		const char *ops = "*+?|()[]-";
		for(const char *c=instr; *c!='\0'; c++)
		{
			lp->str = malloc(2);
			lp->str[0] = *c;
			lp->str[1] = '\0';
			//lp->str = strdup((char[]){*c, '\0'});
			if(strchr(ops, *c))
			{
					lp->is_ident = false;
			}
			else if((*c>='a' && *c<='z') || (*c>='A' && *c<='Z'))
			{
					lp->is_ident = true;
					lp->ident_id = 0;
			}
			else if(*c>='0' && *c<='9')
			{
					lp->is_ident = true;
					lp->ident_id = 1;
			}
			else assert(0);

			lp++;
		}

		lp->str = NULL;
		return l;
}