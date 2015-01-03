
#ifndef __g_cclosure_user_marshal_MARSHAL_H__
#define __g_cclosure_user_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* NONE:INT,INT,INT,INT (gtkcboard-marshal.list:1) */
extern void g_cclosure_user_marshal_VOID__INT_INT_INT_INT (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data);
#define g_cclosure_user_marshal_NONE__INT_INT_INT_INT	g_cclosure_user_marshal_VOID__INT_INT_INT_INT

/* BOOLEAN:INT,INT,INT,INT,INT,INT (gtkcboard-marshal.list:2) */
extern void g_cclosure_user_marshal_BOOLEAN__INT_INT_INT_INT_INT_INT (GClosure     *closure,
                                                                      GValue       *return_value,
                                                                      guint         n_param_values,
                                                                      const GValue *param_values,
                                                                      gpointer      invocation_hint,
                                                                      gpointer      marshal_data);

/* NONE:NONE (gtkcboard-marshal.list:3) */
#define g_cclosure_user_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID
#define g_cclosure_user_marshal_NONE__NONE	g_cclosure_user_marshal_VOID__VOID

G_END_DECLS

#endif /* __g_cclosure_user_marshal_MARSHAL_H__ */

