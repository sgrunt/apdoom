using APDoomLauncher.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace APDoomLauncher
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
            this.AcceptButton = btnLaunchDOOM;
        }

        private void pictureBox1_Paint(object sender, PaintEventArgs paintEventArgs)
        {
            paintEventArgs.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            base.OnPaint(paintEventArgs);
        }

        private void pictureBox2_Paint(object sender, PaintEventArgs paintEventArgs)
        {
            paintEventArgs.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            base.OnPaint(paintEventArgs);
        }

        private void btnLaunchDOOM_Click(object sender, EventArgs e)
        {
            Settings.Default.Game = cboGame.SelectedIndex;
            Settings.Default.Fullscreen = chkFullscreen.Checked;
            Settings.Default.Width = txtWidth.Text;
            Settings.Default.Height = txtHeight.Text;
            Settings.Default.CommandLine = txtCommandLine.Text;
            Settings.Default.Server = txtServer.Text;
            Settings.Default.Player = txtPlayer.Text;
            Settings.Default.Password = txtPassword.Text;
            Settings.Default.Save();

            string command_line = $"-apserver {txtServer.Text} -applayer {txtPlayer.Text}";
            if (txtPassword.Text.Count() > 0)
            {
                command_line += $" -password {txtPassword.Text}";
            }

            switch (cboGame.SelectedIndex)
            {
                case 0: command_line += " -game doom"; break;
                case 1: command_line += " -game doom2"; break;
            }

            if (chkFullscreen.Checked)
            {
                command_line += " -fullscreen";
            }
            else
            {
                command_line += $" -nofullscreen -width {txtWidth.Text} -height {txtHeight.Text}";
            }
            if (txtCommandLine.Text != "") command_line += " " + txtCommandLine.Text;
            System.Diagnostics.Process.Start("crispy-apdoom.exe", command_line);

            Environment.Exit(0);
        }

        private void chkFullscreen_CheckedChanged(object sender, EventArgs e)
        {
            txtWidth.Enabled = !chkFullscreen.Checked;
            txtHeight.Enabled = !chkFullscreen.Checked;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            cboGame.SelectedIndex = Settings.Default.Game;
            chkFullscreen.Checked = Settings.Default.Fullscreen;
            txtWidth.Text = Settings.Default.Width;
            txtHeight.Text = Settings.Default.Height;
            txtCommandLine.Text = Settings.Default.CommandLine;
            txtServer.Text = Settings.Default.Server;
            txtPlayer.Text = Settings.Default.Player;
            txtPassword.Text = Settings.Default.Password;

            txtWidth.Enabled = !chkFullscreen.Checked;
            txtHeight.Enabled = !chkFullscreen.Checked;

            pictureBox1.Visible = false;
            pictureBox2.Visible = false;

            switch (cboGame.SelectedIndex)
            {
                case 0: pictureBox1.Visible = true; break;
                case 1: pictureBox2.Visible = true; break;
            }
        }

        private void cboGame_SelectedIndexChanged(object sender, EventArgs e)
        {
            pictureBox1.Visible = false;
            pictureBox2.Visible = false;

            switch (cboGame.SelectedIndex)
            {
                case 0: pictureBox1.Visible = true; break;
                case 1: pictureBox2.Visible = true; break;
            }
        }
    }
}
