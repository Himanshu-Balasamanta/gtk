#include <gtk/gtk.h>

const char *
widget_name (GtkWidget *widget)
{
  if (gtk_widget_get_name (widget))
    return gtk_widget_get_name (widget);
  else if (GTK_IS_LABEL (widget))
    return gtk_label_get_label (GTK_LABEL (widget));
  else if (GTK_IS_EDITABLE (widget))
    return gtk_editable_get_text (GTK_EDITABLE (widget));
  else
    return G_OBJECT_TYPE_NAME (widget);
}

static void
focus_in (GtkEventControllerFocus *key,
          GString                 *s)
{
  GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (key));

  g_string_append_printf (s, "%s: focus-in is-focus: %d contains-focus: %d\n",
                          widget_name (widget),
                          gtk_event_controller_focus_is_focus (key),
                          gtk_event_controller_focus_contains_focus (key));
}

static void
focus_out (GtkEventControllerFocus *key,
           GString                 *s)
{
  GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (key));

  g_string_append_printf (s, "%s: focus-out is-focus: %d contains-focus: %d\n",
                          widget_name (widget),
                          gtk_event_controller_focus_is_focus (key),
                          gtk_event_controller_focus_contains_focus (key));
}

static void
notify (GtkEventControllerFocus *key,
        GParamSpec              *pspec,
        GString                 *s)
{
  GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (key));

  g_string_append_printf (s, "%s: notify:%s is-focus: %d contains-focus: %d\n",
                          widget_name (widget),
                          pspec->name,
                          gtk_event_controller_focus_is_focus (key),
                          gtk_event_controller_focus_contains_focus (key));
}

static void
add_controller (GtkWidget *widget, GString *s)
{
  GtkEventController *controller;

  controller = gtk_event_controller_focus_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (focus_in), s);
  g_signal_connect (controller, "leave", G_CALLBACK (focus_out), s);
  g_signal_connect (controller, "notify", G_CALLBACK (notify), s);
  gtk_widget_add_controller (widget, controller);
}

static void
assert_result (const char *s, const char *expected)
{
  if (strcmp (s, expected) != 0 && g_test_verbose ())
    {
      g_print ("Expected:\n%s", expected);
      g_print ("Got:\n%s", s);
    }

  g_assert_cmpstr (s, ==, expected);
}

static void
test_window_focus (void)
{
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *entry1;
  GtkWidget *entry2;
  GString *s = g_string_new ("");

  /*
   * The tree look like this, with [] indicating
   * focus locations:
   *
   *       window
   *         |
   *  +----[box]-----+
   *  |      |       |
   * label1 box1    box2------+
   *         |       |        |
   *      [entry1]  label2  [entry2]
   */

  window = gtk_window_new ();
  gtk_widget_set_name (window, "window");
  add_controller (window, s);
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_can_focus (box, TRUE);
  gtk_widget_set_name (box, "box");
  add_controller (box, s);
  gtk_container_add (GTK_CONTAINER (window), box);
  label1 = gtk_label_new ("label1");
  gtk_widget_set_name (label1, "label1");
  add_controller (label1, s);
  gtk_container_add (GTK_CONTAINER (box), label1);
  box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_name (box1, "box1");
  add_controller (box1, s);
  gtk_container_add (GTK_CONTAINER (box), box1);
  entry1 = gtk_text_new ();
  gtk_widget_set_name (entry1, "entry1");
  add_controller (entry1, s);
  gtk_container_add (GTK_CONTAINER (box1), entry1);
  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_name (box2, "box2");
  add_controller (box2, s);
  gtk_container_add (GTK_CONTAINER (box), box2);
  label2 = gtk_label_new ("label2");
  gtk_widget_set_name (label2, "label2");
  add_controller (label2, s);
  gtk_container_add (GTK_CONTAINER (box2), label2);
  entry2 = gtk_text_new ();
  gtk_widget_set_name (entry2, "entry2");
  add_controller (entry2, s);
  gtk_container_add (GTK_CONTAINER (box2), entry2);

  g_assert_null (gtk_window_get_focus (GTK_WINDOW (window)));

  gtk_widget_show (window);

  /* show puts the initial focus on box */
  g_assert (gtk_window_get_focus (GTK_WINDOW (window)) == box);

  if (g_test_verbose ())
    g_print ("-> box\n%s\n", s->str);

  assert_result (s->str,
                 "window: notify:contains-focus is-focus: 0 contains-focus: 1\n"
                 "window: focus-in is-focus: 0 contains-focus: 1\n"
                 "box: notify:contains-focus is-focus: 1 contains-focus: 1\n"
                 "box: notify:is-focus is-focus: 1 contains-focus: 1\n"
                 "box: focus-in is-focus: 1 contains-focus: 1\n");
  g_string_truncate (s, 0);

  gtk_widget_grab_focus (entry1);

  if (g_test_verbose ())
    g_print ("box -> entry1\n%s\n", s->str);

  assert_result (s->str,
                 "box: notify:is-focus is-focus: 0 contains-focus: 1\n"
                 "box1: notify:contains-focus is-focus: 0 contains-focus: 1\n"
                 "box1: focus-in is-focus: 0 contains-focus: 1\n"
                 "entry1: notify:contains-focus is-focus: 1 contains-focus: 1\n"
                 "entry1: notify:is-focus is-focus: 1 contains-focus: 1\n"
                 "entry1: focus-in is-focus: 1 contains-focus: 1\n");

  g_string_truncate (s, 0);

  g_assert (gtk_window_get_focus (GTK_WINDOW (window)) == entry1);

  gtk_widget_grab_focus (entry2);

  if (g_test_verbose ())
    g_print ("entry1 -> entry2\n%s\n", s->str);

  assert_result (s->str,
                 "entry1: focus-out is-focus: 1 contains-focus: 1\n"
                 "entry1: notify:contains-focus is-focus: 0 contains-focus: 0\n"
                 "entry1: notify:is-focus is-focus: 0 contains-focus: 0\n"
                 "box1: focus-out is-focus: 0 contains-focus: 1\n"
                 "box1: notify:contains-focus is-focus: 0 contains-focus: 0\n"
                 "box2: notify:contains-focus is-focus: 0 contains-focus: 1\n"
                 "box2: focus-in is-focus: 0 contains-focus: 1\n"
                 "entry2: notify:contains-focus is-focus: 1 contains-focus: 1\n"
                 "entry2: notify:is-focus is-focus: 1 contains-focus: 1\n"
                 "entry2: focus-in is-focus: 1 contains-focus: 1\n");

  g_string_truncate (s, 0);

  g_assert (gtk_window_get_focus (GTK_WINDOW (window)) == entry2);

  gtk_widget_grab_focus (box);

  if (g_test_verbose ())
    g_print ("entry2 -> box\n%s", s->str);

  assert_result (s->str,
                 "entry2: focus-out is-focus: 1 contains-focus: 1\n"
                 "entry2: notify:contains-focus is-focus: 0 contains-focus: 0\n"
                 "entry2: notify:is-focus is-focus: 0 contains-focus: 0\n"
                 "box2: focus-out is-focus: 0 contains-focus: 1\n"
                 "box2: notify:contains-focus is-focus: 0 contains-focus: 0\n"
                 "box: notify:is-focus is-focus: 1 contains-focus: 1\n");

  g_string_truncate (s, 0);

  gtk_widget_hide (window);

  g_assert (gtk_window_get_focus (GTK_WINDOW (window)) == box);
   
  gtk_window_set_focus (GTK_WINDOW (window), entry1);

  g_assert (gtk_window_get_focus (GTK_WINDOW (window)) == entry1);

  gtk_widget_destroy (window);
}

int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv);

  g_test_add_func ("/focus/window", test_window_focus);

  return g_test_run ();
}
