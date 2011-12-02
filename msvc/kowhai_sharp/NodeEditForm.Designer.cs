﻿namespace kowhai_sharp
{
    partial class NodeEditForm
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
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnWrite = new System.Windows.Forms.Button();
            this.tbHex = new System.Windows.Forms.TextBox();
            this.kowhaiTree1 = new kowhai_sharp.KowhaiTree();
            this.SuspendLayout();
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(199, 255);
            this.btnCancel.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 31);
            this.btnCancel.TabIndex = 3;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // btnWrite
            // 
            this.btnWrite.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnWrite.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnWrite.Location = new System.Drawing.Point(119, 255);
            this.btnWrite.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnWrite.Name = "btnWrite";
            this.btnWrite.Size = new System.Drawing.Size(75, 31);
            this.btnWrite.TabIndex = 2;
            this.btnWrite.Text = "Write";
            this.btnWrite.UseVisualStyleBackColor = true;
            // 
            // tbHex
            // 
            this.tbHex.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbHex.Font = new System.Drawing.Font("Courier New", 7.8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tbHex.Location = new System.Drawing.Point(15, 224);
            this.tbHex.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.tbHex.Name = "tbHex";
            this.tbHex.Size = new System.Drawing.Size(257, 22);
            this.tbHex.TabIndex = 1;
            this.tbHex.Validated += new System.EventHandler(this.tbHex_Validated);
            // 
            // kowhaiTree1
            // 
            this.kowhaiTree1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.kowhaiTree1.ContextMenuEnabled = true;
            this.kowhaiTree1.Location = new System.Drawing.Point(15, 14);
            this.kowhaiTree1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.kowhaiTree1.Name = "kowhaiTree1";
            this.kowhaiTree1.Size = new System.Drawing.Size(259, 204);
            this.kowhaiTree1.TabIndex = 0;
            // 
            // NodeEditForm
            // 
            this.AcceptButton = this.btnWrite;
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(283, 299);
            this.Controls.Add(this.tbHex);
            this.Controls.Add(this.btnWrite);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.kowhaiTree1);
            this.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.Name = "NodeEditForm";
            this.Text = "NodeEditForm";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private KowhaiTree kowhaiTree1;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnWrite;
        private System.Windows.Forms.TextBox tbHex;
    }
}