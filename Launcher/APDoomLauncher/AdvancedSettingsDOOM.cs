using APDoomLauncher.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace APDoomLauncher
{
    public partial class AdvancedSettingsDOOM : Form
    {
        public AdvancedSettingsDOOM(int game)
        {
            InitializeComponent();

            pnlFlipLevels.Enabled = game != 2; // Heretic doesn't flip levels

            chkDifficulty_useServer.Checked = !Settings.Default.OverrideDifficulty;
            chkMonsterRando_userServer.Checked = !Settings.Default.OverrideMonsterRando;
            chkItemRando_userServer.Checked = !Settings.Default.OverrideItemRando;
            chkMusicRando_userServer.Checked = !Settings.Default.OverrideMusicRando;
            chkFlipLevels_userServer.Checked = !Settings.Default.OverrideFlipLevels;
            chkResetLevel_useServer.Checked = !Settings.Default.OverrideResetLevel;

            cboDifficulty.Enabled = !chkDifficulty_useServer.Checked;
            cboMonsterRando.Enabled = !chkMonsterRando_userServer.Checked;
            cboItemRando.Enabled = !chkItemRando_userServer.Checked;
            cboMusicRando.Enabled = !chkMusicRando_userServer.Checked;
            cboFlipLevels.Enabled = !chkFlipLevels_userServer.Checked;
            chkResetLevels.Enabled = !chkResetLevel_useServer.Checked;

            cboDifficulty.SelectedIndex = Settings.Default.Difficulty;
            cboMonsterRando.SelectedIndex = Settings.Default.MonsterRando;
            cboItemRando.SelectedIndex = Settings.Default.ItemRando;
            cboMusicRando.SelectedIndex = Settings.Default.MusicRando;
            cboFlipLevels.SelectedIndex = Settings.Default.FlipLevels;
            chkDeathLink.Checked = Settings.Default.ForceDeathLinkOff;
            chkResetLevels.Checked = Settings.Default.ResetLevels;
            chkFastMonsters.Checked = Settings.Default.FastMonsters;
        }

        private void btnDefaults_Click(object sender, EventArgs e)
        {
            chkDifficulty_useServer.Checked = true;
            chkMonsterRando_userServer.Checked = true;
            chkItemRando_userServer.Checked = true;
            chkMusicRando_userServer.Checked = true;
            chkFlipLevels_userServer.Checked = true;
            chkResetLevel_useServer.Checked = true;

            cboDifficulty.Enabled = false;
            cboMonsterRando.Enabled = false;
            cboItemRando.Enabled = false;
            cboMusicRando.Enabled = false;
            cboFlipLevels.Enabled = false;
            chkDeathLink.Enabled = false;
            chkResetLevels.Enabled = false;

            cboDifficulty.SelectedIndex = 2;
            cboMonsterRando.SelectedIndex = 1;
            cboItemRando.SelectedIndex = 1;
            cboMusicRando.SelectedIndex = 0;
            cboFlipLevels.SelectedIndex = 0;
            chkDeathLink.Checked = false;
            chkResetLevels.Checked = true;
            chkFastMonsters.Checked = false;
        }

        private void chkDifficulty_useServer_CheckedChanged(object sender, EventArgs e)
        {
            cboDifficulty.Enabled = !chkDifficulty_useServer.Checked;
        }

        private void chkMonsterRando_userServer_CheckedChanged(object sender, EventArgs e)
        {
            cboMonsterRando.Enabled = !chkMonsterRando_userServer.Checked;
        }

        private void chkItemRando_userServer_CheckedChanged(object sender, EventArgs e)
        {
            cboItemRando.Enabled = !chkItemRando_userServer.Checked;
        }

        private void chkMusicRando_userServer_CheckedChanged(object sender, EventArgs e)
        {
            cboMusicRando.Enabled = !chkMusicRando_userServer.Checked;
        }

        private void chkFlipLevels_userServer_CheckedChanged(object sender, EventArgs e)
        {
            cboFlipLevels.Enabled = !chkFlipLevels_userServer.Checked;
        }

        private void chkResetLevel_useServer_CheckedChanged(object sender, EventArgs e)
        {
            chkResetLevels.Enabled = !chkResetLevel_useServer.Checked;
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            Settings.Default.OverrideDifficulty = !chkDifficulty_useServer.Checked;
            Settings.Default.OverrideMonsterRando = !chkMonsterRando_userServer.Checked;
            Settings.Default.OverrideItemRando = !chkItemRando_userServer.Checked;
            Settings.Default.OverrideMusicRando = !chkMusicRando_userServer.Checked;
            Settings.Default.OverrideFlipLevels = !chkFlipLevels_userServer.Checked;
            Settings.Default.OverrideResetLevel = !chkResetLevel_useServer.Checked;

            Settings.Default.Difficulty = cboDifficulty.SelectedIndex;
            Settings.Default.MonsterRando = cboMonsterRando.SelectedIndex;
            Settings.Default.ItemRando = cboItemRando.SelectedIndex;
            Settings.Default.MusicRando = cboMusicRando.SelectedIndex;
            Settings.Default.FlipLevels = cboFlipLevels.SelectedIndex;
            Settings.Default.ForceDeathLinkOff = chkDeathLink.Checked;
            Settings.Default.ResetLevels = chkResetLevels.Checked;
            Settings.Default.FastMonsters = chkFastMonsters.Checked;

            this.Close();
        }
    }
}
