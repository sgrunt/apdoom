
namespace APDoomLauncher
{
    partial class AdvancedSettingsDOOM
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AdvancedSettingsDOOM));
            this.btnClose = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.chkDifficulty_useServer = new System.Windows.Forms.CheckBox();
            this.cboMonsterRando = new System.Windows.Forms.ComboBox();
            this.chkMonsterRando_userServer = new System.Windows.Forms.CheckBox();
            this.cboItemRando = new System.Windows.Forms.ComboBox();
            this.chkItemRando_userServer = new System.Windows.Forms.CheckBox();
            this.cboFlipLevels = new System.Windows.Forms.ComboBox();
            this.chkFlipLevels_userServer = new System.Windows.Forms.CheckBox();
            this.chkDeathLink = new System.Windows.Forms.CheckBox();
            this.chkResetLevels = new System.Windows.Forms.CheckBox();
            this.chkResetLevel_useServer = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.panel3 = new System.Windows.Forms.Panel();
            this.pnlFlipLevels = new System.Windows.Forms.Panel();
            this.panel5 = new System.Windows.Forms.Panel();
            this.panel6 = new System.Windows.Forms.Panel();
            this.label6 = new System.Windows.Forms.Label();
            this.btnDefaults = new System.Windows.Forms.Button();
            this.cboDifficulty = new System.Windows.Forms.ComboBox();
            this.panel7 = new System.Windows.Forms.Panel();
            this.label7 = new System.Windows.Forms.Label();
            this.chkFastMonsters = new System.Windows.Forms.CheckBox();
            this.panel4 = new System.Windows.Forms.Panel();
            this.label8 = new System.Windows.Forms.Label();
            this.chkMusicRando_userServer = new System.Windows.Forms.CheckBox();
            this.cboMusicRando = new System.Windows.Forms.ComboBox();
            this.panel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.panel3.SuspendLayout();
            this.pnlFlipLevels.SuspendLayout();
            this.panel5.SuspendLayout();
            this.panel6.SuspendLayout();
            this.panel7.SuspendLayout();
            this.panel4.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnClose.Location = new System.Drawing.Point(744, 364);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 31);
            this.btnClose.TabIndex = 0;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(53, 10);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(61, 17);
            this.label1.TabIndex = 5;
            this.label1.Text = "Difficulty";
            // 
            // chkDifficulty_useServer
            // 
            this.chkDifficulty_useServer.AutoSize = true;
            this.chkDifficulty_useServer.Checked = true;
            this.chkDifficulty_useServer.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkDifficulty_useServer.Location = new System.Drawing.Point(310, 9);
            this.chkDifficulty_useServer.Name = "chkDifficulty_useServer";
            this.chkDifficulty_useServer.Size = new System.Drawing.Size(145, 21);
            this.chkDifficulty_useServer.TabIndex = 6;
            this.chkDifficulty_useServer.Text = "Use server setting";
            this.chkDifficulty_useServer.UseVisualStyleBackColor = true;
            this.chkDifficulty_useServer.CheckedChanged += new System.EventHandler(this.chkDifficulty_useServer_CheckedChanged);
            // 
            // cboMonsterRando
            // 
            this.cboMonsterRando.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboMonsterRando.Enabled = false;
            this.cboMonsterRando.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.cboMonsterRando.FormattingEnabled = true;
            this.cboMonsterRando.Items.AddRange(new object[] {
            "Vanilla",
            "Shuffle",
            "Random balanced",
            "Random chaotic"});
            this.cboMonsterRando.Location = new System.Drawing.Point(557, 83);
            this.cboMonsterRando.Name = "cboMonsterRando";
            this.cboMonsterRando.Size = new System.Drawing.Size(227, 24);
            this.cboMonsterRando.TabIndex = 9;
            // 
            // chkMonsterRando_userServer
            // 
            this.chkMonsterRando_userServer.AutoSize = true;
            this.chkMonsterRando_userServer.Checked = true;
            this.chkMonsterRando_userServer.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkMonsterRando_userServer.Location = new System.Drawing.Point(310, 9);
            this.chkMonsterRando_userServer.Name = "chkMonsterRando_userServer";
            this.chkMonsterRando_userServer.Size = new System.Drawing.Size(145, 21);
            this.chkMonsterRando_userServer.TabIndex = 8;
            this.chkMonsterRando_userServer.Text = "Use server setting";
            this.chkMonsterRando_userServer.UseVisualStyleBackColor = true;
            this.chkMonsterRando_userServer.CheckedChanged += new System.EventHandler(this.chkMonsterRando_userServer_CheckedChanged);
            // 
            // cboItemRando
            // 
            this.cboItemRando.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboItemRando.Enabled = false;
            this.cboItemRando.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.cboItemRando.FormattingEnabled = true;
            this.cboItemRando.Items.AddRange(new object[] {
            "Vanilla",
            "Shuffle",
            "Random balanced"});
            this.cboItemRando.Location = new System.Drawing.Point(557, 119);
            this.cboItemRando.Name = "cboItemRando";
            this.cboItemRando.Size = new System.Drawing.Size(227, 24);
            this.cboItemRando.TabIndex = 11;
            // 
            // chkItemRando_userServer
            // 
            this.chkItemRando_userServer.AutoSize = true;
            this.chkItemRando_userServer.BackColor = System.Drawing.SystemColors.ControlLight;
            this.chkItemRando_userServer.Checked = true;
            this.chkItemRando_userServer.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkItemRando_userServer.Location = new System.Drawing.Point(311, 121);
            this.chkItemRando_userServer.Name = "chkItemRando_userServer";
            this.chkItemRando_userServer.Size = new System.Drawing.Size(145, 21);
            this.chkItemRando_userServer.TabIndex = 10;
            this.chkItemRando_userServer.Text = "Use server setting";
            this.chkItemRando_userServer.UseVisualStyleBackColor = false;
            this.chkItemRando_userServer.CheckedChanged += new System.EventHandler(this.chkItemRando_userServer_CheckedChanged);
            // 
            // cboFlipLevels
            // 
            this.cboFlipLevels.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboFlipLevels.Enabled = false;
            this.cboFlipLevels.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.cboFlipLevels.FormattingEnabled = true;
            this.cboFlipLevels.Items.AddRange(new object[] {
            "Vanilla",
            "Flipped",
            "Randomly flipped"});
            this.cboFlipLevels.Location = new System.Drawing.Point(556, 7);
            this.cboFlipLevels.Name = "cboFlipLevels";
            this.cboFlipLevels.Size = new System.Drawing.Size(227, 24);
            this.cboFlipLevels.TabIndex = 13;
            // 
            // chkFlipLevels_userServer
            // 
            this.chkFlipLevels_userServer.AutoSize = true;
            this.chkFlipLevels_userServer.Checked = true;
            this.chkFlipLevels_userServer.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkFlipLevels_userServer.Location = new System.Drawing.Point(310, 8);
            this.chkFlipLevels_userServer.Name = "chkFlipLevels_userServer";
            this.chkFlipLevels_userServer.Size = new System.Drawing.Size(145, 21);
            this.chkFlipLevels_userServer.TabIndex = 12;
            this.chkFlipLevels_userServer.Text = "Use server setting";
            this.chkFlipLevels_userServer.UseVisualStyleBackColor = true;
            this.chkFlipLevels_userServer.CheckedChanged += new System.EventHandler(this.chkFlipLevels_userServer_CheckedChanged);
            // 
            // chkDeathLink
            // 
            this.chkDeathLink.AutoSize = true;
            this.chkDeathLink.BackColor = System.Drawing.SystemColors.Control;
            this.chkDeathLink.Location = new System.Drawing.Point(556, 9);
            this.chkDeathLink.Name = "chkDeathLink";
            this.chkDeathLink.Size = new System.Drawing.Size(86, 21);
            this.chkDeathLink.TabIndex = 15;
            this.chkDeathLink.Text = "Force off";
            this.chkDeathLink.UseVisualStyleBackColor = false;
            // 
            // chkResetLevels
            // 
            this.chkResetLevels.AutoSize = true;
            this.chkResetLevels.Checked = true;
            this.chkResetLevels.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkResetLevels.Enabled = false;
            this.chkResetLevels.Location = new System.Drawing.Point(556, 9);
            this.chkResetLevels.Name = "chkResetLevels";
            this.chkResetLevels.Size = new System.Drawing.Size(49, 21);
            this.chkResetLevels.TabIndex = 17;
            this.chkResetLevels.Text = "On";
            this.chkResetLevels.UseVisualStyleBackColor = true;
            // 
            // chkResetLevel_useServer
            // 
            this.chkResetLevel_useServer.AutoSize = true;
            this.chkResetLevel_useServer.Checked = true;
            this.chkResetLevel_useServer.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkResetLevel_useServer.Location = new System.Drawing.Point(310, 8);
            this.chkResetLevel_useServer.Name = "chkResetLevel_useServer";
            this.chkResetLevel_useServer.Size = new System.Drawing.Size(145, 21);
            this.chkResetLevel_useServer.TabIndex = 16;
            this.chkResetLevel_useServer.Text = "Use server setting";
            this.chkResetLevel_useServer.UseVisualStyleBackColor = true;
            this.chkResetLevel_useServer.CheckedChanged += new System.EventHandler(this.chkResetLevel_useServer_CheckedChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(53, 10);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(123, 17);
            this.label2.TabIndex = 5;
            this.label2.Text = "Random monsters";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(53, 10);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(113, 17);
            this.label3.TabIndex = 5;
            this.label3.Text = "Random pickups";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(54, 9);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(70, 17);
            this.label4.TabIndex = 5;
            this.label4.Text = "Flip levels";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(53, 9);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(71, 17);
            this.label5.TabIndex = 5;
            this.label5.Text = "Death link";
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel1.Controls.Add(this.cboDifficulty);
            this.panel1.Controls.Add(this.chkDifficulty_useServer);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Location = new System.Drawing.Point(1, 40);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(855, 37);
            this.panel1.TabIndex = 18;
            // 
            // panel2
            // 
            this.panel2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel2.BackColor = System.Drawing.SystemColors.Control;
            this.panel2.Controls.Add(this.chkMonsterRando_userServer);
            this.panel2.Controls.Add(this.label2);
            this.panel2.Location = new System.Drawing.Point(1, 76);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(855, 37);
            this.panel2.TabIndex = 18;
            // 
            // panel3
            // 
            this.panel3.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel3.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel3.Controls.Add(this.label3);
            this.panel3.Location = new System.Drawing.Point(1, 112);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(855, 37);
            this.panel3.TabIndex = 18;
            // 
            // pnlFlipLevels
            // 
            this.pnlFlipLevels.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.pnlFlipLevels.BackColor = System.Drawing.SystemColors.ControlLight;
            this.pnlFlipLevels.Controls.Add(this.chkFlipLevels_userServer);
            this.pnlFlipLevels.Controls.Add(this.cboFlipLevels);
            this.pnlFlipLevels.Controls.Add(this.label4);
            this.pnlFlipLevels.Location = new System.Drawing.Point(1, 184);
            this.pnlFlipLevels.Name = "pnlFlipLevels";
            this.pnlFlipLevels.Size = new System.Drawing.Size(855, 37);
            this.pnlFlipLevels.TabIndex = 18;
            // 
            // panel5
            // 
            this.panel5.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel5.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel5.Controls.Add(this.chkDeathLink);
            this.panel5.Controls.Add(this.label5);
            this.panel5.Location = new System.Drawing.Point(1, 256);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(855, 37);
            this.panel5.TabIndex = 18;
            // 
            // panel6
            // 
            this.panel6.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel6.BackColor = System.Drawing.SystemColors.Control;
            this.panel6.Controls.Add(this.chkResetLevels);
            this.panel6.Controls.Add(this.label6);
            this.panel6.Controls.Add(this.chkResetLevel_useServer);
            this.panel6.Location = new System.Drawing.Point(1, 220);
            this.panel6.Name = "panel6";
            this.panel6.Size = new System.Drawing.Size(855, 37);
            this.panel6.TabIndex = 18;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(53, 9);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(138, 17);
            this.label6.TabIndex = 5;
            this.label6.Text = "Reset level on death";
            // 
            // btnDefaults
            // 
            this.btnDefaults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnDefaults.Location = new System.Drawing.Point(40, 364);
            this.btnDefaults.Name = "btnDefaults";
            this.btnDefaults.Size = new System.Drawing.Size(137, 31);
            this.btnDefaults.TabIndex = 0;
            this.btnDefaults.Text = "Reset defaults";
            this.btnDefaults.UseVisualStyleBackColor = true;
            this.btnDefaults.Click += new System.EventHandler(this.btnDefaults_Click);
            // 
            // cboDifficulty
            // 
            this.cboDifficulty.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboDifficulty.Enabled = false;
            this.cboDifficulty.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.cboDifficulty.FormattingEnabled = true;
            this.cboDifficulty.Items.AddRange(new object[] {
            "Baby",
            "Easy",
            "Medium",
            "Hard",
            "Nightmare"});
            this.cboDifficulty.Location = new System.Drawing.Point(556, 6);
            this.cboDifficulty.Name = "cboDifficulty";
            this.cboDifficulty.Size = new System.Drawing.Size(227, 24);
            this.cboDifficulty.TabIndex = 7;
            // 
            // panel7
            // 
            this.panel7.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel7.BackColor = System.Drawing.SystemColors.Control;
            this.panel7.Controls.Add(this.chkFastMonsters);
            this.panel7.Controls.Add(this.label7);
            this.panel7.Location = new System.Drawing.Point(1, 292);
            this.panel7.Name = "panel7";
            this.panel7.Size = new System.Drawing.Size(855, 37);
            this.panel7.TabIndex = 18;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(54, 9);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(97, 17);
            this.label7.TabIndex = 5;
            this.label7.Text = "Fast monsters";
            // 
            // chkFastMonsters
            // 
            this.chkFastMonsters.AutoSize = true;
            this.chkFastMonsters.Location = new System.Drawing.Point(556, 8);
            this.chkFastMonsters.Name = "chkFastMonsters";
            this.chkFastMonsters.Size = new System.Drawing.Size(49, 21);
            this.chkFastMonsters.TabIndex = 18;
            this.chkFastMonsters.Text = "On";
            this.chkFastMonsters.UseVisualStyleBackColor = true;
            // 
            // panel4
            // 
            this.panel4.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel4.BackColor = System.Drawing.SystemColors.Control;
            this.panel4.Controls.Add(this.cboMusicRando);
            this.panel4.Controls.Add(this.chkMusicRando_userServer);
            this.panel4.Controls.Add(this.label8);
            this.panel4.Location = new System.Drawing.Point(1, 148);
            this.panel4.Name = "panel4";
            this.panel4.Size = new System.Drawing.Size(855, 37);
            this.panel4.TabIndex = 18;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(53, 10);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(101, 17);
            this.label8.TabIndex = 5;
            this.label8.Text = "Random music";
            // 
            // chkMusicRando_userServer
            // 
            this.chkMusicRando_userServer.AutoSize = true;
            this.chkMusicRando_userServer.Checked = true;
            this.chkMusicRando_userServer.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkMusicRando_userServer.Location = new System.Drawing.Point(310, 9);
            this.chkMusicRando_userServer.Name = "chkMusicRando_userServer";
            this.chkMusicRando_userServer.Size = new System.Drawing.Size(145, 21);
            this.chkMusicRando_userServer.TabIndex = 13;
            this.chkMusicRando_userServer.Text = "Use server setting";
            this.chkMusicRando_userServer.UseVisualStyleBackColor = true;
            this.chkMusicRando_userServer.CheckedChanged += new System.EventHandler(this.chkMusicRando_userServer_CheckedChanged);
            // 
            // cboMusicRando
            // 
            this.cboMusicRando.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboMusicRando.Enabled = false;
            this.cboMusicRando.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.cboMusicRando.FormattingEnabled = true;
            this.cboMusicRando.Items.AddRange(new object[] {
            "Vanilla",
            "Shuffle Selected",
            "Shuffle Game"});
            this.cboMusicRando.Location = new System.Drawing.Point(556, 7);
            this.cboMusicRando.Name = "cboMusicRando";
            this.cboMusicRando.Size = new System.Drawing.Size(227, 24);
            this.cboMusicRando.TabIndex = 14;
            // 
            // AdvancedSettingsDOOM
            // 
            this.AcceptButton = this.btnClose;
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(857, 423);
            this.Controls.Add(this.panel7);
            this.Controls.Add(this.cboItemRando);
            this.Controls.Add(this.chkItemRando_userServer);
            this.Controls.Add(this.cboMonsterRando);
            this.Controls.Add(this.btnDefaults);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.panel6);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.pnlFlipLevels);
            this.Controls.Add(this.panel4);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.panel2);
            this.Controls.Add(this.panel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "AdvancedSettingsDOOM";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Advanced settings";
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.panel3.ResumeLayout(false);
            this.panel3.PerformLayout();
            this.pnlFlipLevels.ResumeLayout(false);
            this.pnlFlipLevels.PerformLayout();
            this.panel5.ResumeLayout(false);
            this.panel5.PerformLayout();
            this.panel6.ResumeLayout(false);
            this.panel6.PerformLayout();
            this.panel7.ResumeLayout(false);
            this.panel7.PerformLayout();
            this.panel4.ResumeLayout(false);
            this.panel4.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox chkDifficulty_useServer;
        private System.Windows.Forms.ComboBox cboMonsterRando;
        private System.Windows.Forms.CheckBox chkMonsterRando_userServer;
        private System.Windows.Forms.ComboBox cboItemRando;
        private System.Windows.Forms.CheckBox chkItemRando_userServer;
        private System.Windows.Forms.ComboBox cboFlipLevels;
        private System.Windows.Forms.CheckBox chkFlipLevels_userServer;
        private System.Windows.Forms.CheckBox chkDeathLink;
        private System.Windows.Forms.CheckBox chkResetLevels;
        private System.Windows.Forms.CheckBox chkResetLevel_useServer;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Panel pnlFlipLevels;
        private System.Windows.Forms.Panel panel5;
        private System.Windows.Forms.Panel panel6;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Button btnDefaults;
        private System.Windows.Forms.ComboBox cboDifficulty;
        private System.Windows.Forms.Panel panel7;
        private System.Windows.Forms.CheckBox chkFastMonsters;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Panel panel4;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.ComboBox cboMusicRando;
        private System.Windows.Forms.CheckBox chkMusicRando_userServer;
    }
}