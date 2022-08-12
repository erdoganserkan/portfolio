
// This function is used for detection of user key press
// Normally if you call getchar() it waits for user to press enter keyword after character entry.
// but this fucntion sets terminal to non-canonical mode, getchar() returns immediately after user pres any character.
unsigned char wait_for_key_input ( char* p_input_key_list )
{
	int		result_key;
	bool		is_continue = true;
	unsigned char i;
	struct termios old_terminal_status, new_terminal_status;

	ASSERT_APP ( (NULL != p_input_key_list), "ERR:p_allowed_key_list" );
	ASSERT_APP ( strlen ( p_input_key_list ) > 0, "ERR:p_allowed_key_list size(min)" );
	ASSERT_APP ( strlen ( p_input_key_list ) <= INPUT_KEY_LIST_SIZE_MAX, "ERR:p_allowed_key_list size(max)" );

	// Set the terminal into non-canonical mode.
	tcgetattr ( STDIN_FILENO, &old_terminal_status );
	new_terminal_status = old_terminal_status;
	new_terminal_status.c_lflag &= ~ ( ICANON | ECHO );
	tcsetattr ( STDIN_FILENO, TCSANOW, &new_terminal_status );

	while ( is_continue ) {
		if ( ( result_key = getchar(  ) ) != EOF ) {
			for ( i = 0; i < strlen ( p_input_key_list ); i++ ) {
				if ( p_input_key_list[i] == ( unsigned char ) result_key ) {
					is_continue = false;
					break;
				}
			}
		}
	}

	// Restore the old terminal status
	tcsetattr ( STDIN_FILENO, TCSANOW, &old_terminal_status );

	return ( unsigned char ) result_key;
}

