


/* --- pspec name display --- */
static BstGMask*
param_pspec_create_gmask (BstParam  *bparam,
			  GtkWidget *gmask_parent)
{
  return bst_gmask_form_big (gmask_parent, g_object_new (GTK_TYPE_LABEL,
							 "visible", TRUE,
							 "xalign", 0.0,
							 NULL));
}

static GtkWidget*
param_pspec_create_widget (BstParam *bparam)
{
  return g_object_new (GTK_TYPE_LABEL,
		       "visible", TRUE,
		       "xalign", 0.5,
		       NULL);
}

static void
param_pspec_update (BstParam  *bparam,
		    GtkWidget *action)
{
  gtk_label_set_text (GTK_LABEL (action), g_param_spec_get_nick (bparam->pspec));
}

struct _BstParamImpl param_pspec = {
  "Property Name",	-100,
  0 /* variant */,	0    /* flags */,
  0 /* scat */,		NULL /* hints */,
  param_pspec_create_gmask,
  NULL,	/* create_widget */
  param_pspec_update,
};

struct _BstParamImpl rack_pspec = {
  "Property Name",	-100,
  0 /* variant */,	0    /* flags */,
  0 /* scat */,		NULL /* hints */,
  NULL, /* create_gmask */
  param_pspec_create_widget,
  param_pspec_update,
};
