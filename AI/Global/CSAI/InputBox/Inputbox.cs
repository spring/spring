/*
 * ajma.Utils.InputBox
 * Displays a prompt in a dialog box, waits for the user to input text or click a button, and then returns a string containing the contents of the text box.
 *  
 * Andrew J. Ma
 * ajmaonline@hotmail.com
 */

using System;
using System.Drawing;

namespace ajma.Utils
{
	/// <summary>
	/// Displays a prompt in a dialog box, waits for the user to input text or click a button, and then returns a string containing the contents of the text box.
	/// </summary>
	public class InputBox
	{
		/// <summary>
		/// Displays a prompt in a dialog box, waits for the user to input text or click a button, and then returns a string containing the contents of the text box.
		/// </summary>
		/// <param name="Prompt">String expression displayed as the message in the dialog box.</param>
		/// <param name="Title">String expression displayed in the title bar of the dialog box.</param>
		/// <returns>The value in the textbox is returned if the user clicks OK or presses the ENTER key. If the user clicks Cancel, a zero-length string is returned.</returns>
		public static string Show(string Prompt, string Title)
		{
			return Show(Prompt, Title, "", -1, -1);
		}

		/// <summary>
		/// Displays a prompt in a dialog box, waits for the user to input text or click a button, and then returns a string containing the contents of the text box.
		/// </summary>
		/// <param name="Prompt">String expression displayed as the message in the dialog box.</param>
		/// <param name="Title">String expression displayed in the title bar of the dialog box.</param>
		/// <param name="DefaultResponse">String expression displayed in the text box as the default response if no other input is provided. If you omit DefaultResponse, the displayed text box is empty.</param>
		/// <returns>The value in the textbox is returned if the user clicks OK or presses the ENTER key. If the user clicks Cancel, a zero-length string is returned.</returns>
		public static string Show(string Prompt, string Title, string DefaultResponse)
		{
			return Show(Prompt, Title, DefaultResponse, -1, -1);
		}

		/// <summary>
		/// Displays a prompt in a dialog box, waits for the user to input text or click a button, and then returns a string containing the contents of the text box.
		/// </summary>
		/// <param name="Prompt">String expression displayed as the message in the dialog box.</param>
		/// <param name="Title">String expression displayed in the title bar of the dialog box.</param>
		/// <param name="DefaultResponse">String expression displayed in the text box as the default response if no other input is provided. If you omit DefaultResponse, the displayed text box is empty.</param>
		/// <param name="XPos">Integer expression that specifies, in pixels, the distance of the left edge of the dialog box from the left edge of the screen.</param>
		/// <param name="YPos">Integer expression that specifies, in pixels, the distance of the upper edge of the dialog box from the top of the screen.</param>
		/// <returns>The value in the textbox is returned if the user clicks OK or presses the ENTER key. If the user clicks Cancel, a zero-length string is returned.</returns>
		public static string Show(string Prompt, string Title, string DefaultResponse, int XPos, int YPos)
		{
			// Create a new input box dialog
			InputBoxForm frmInputBox = new InputBoxForm();
			frmInputBox.Title = Title;
			frmInputBox.Prompt = Prompt;
			frmInputBox.DefaultResponse = DefaultResponse;
			if (XPos >= 0 && YPos >= 0)  frmInputBox.StartLocation = new Point(XPos, YPos);
			frmInputBox.ShowDialog();
			return frmInputBox.ReturnValue;
		}
	}
}
