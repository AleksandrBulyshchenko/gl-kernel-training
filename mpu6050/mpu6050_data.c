#include "mpu6050_data.h"
#include <linux/spinlock.h>

struct mpu6050_data_holder_internal {
	struct mpu6050_data_holder public_holder;
	size_t elements_count;
	struct mpu6050_data_list *element_iter_current;
	struct mpu6050_data_list *element_iter;
	struct mpu6050_data_list list;
	spinlock_t lock;
};

static struct mpu6050_data_holder_internal g_mpu6050_data = {
	.lock = __SPIN_LOCK_UNLOCKED(lock)
};

struct mpu6050_data_holder *get_mpu6050_data(void)
{
	return &g_mpu6050_data.public_holder;
}

void init_mpu6050_data(struct mpu6050_data_holder *data_,
	size_t elements_count)
{
	size_t const element_size = sizeof(struct mpu6050_data_list);
	struct mpu6050_data_holder_internal *data =
		(struct mpu6050_data_holder_internal *)data_;

	if (!data)
		return;
	data->elements_count = 0;
	data->element_iter_current = NULL;
	data->element_iter = NULL;

	INIT_LIST_HEAD(&data->list.list);

	do {
		struct mpu6050_data_list *element = kzalloc(
			element_size, GFP_KERNEL);

		pr_info("%s %s: element %p\n",
			THIS_MODULE->name, __func__, element);
		if (!element)
			break;
		INIT_LIST_HEAD(&element->list);
		list_add(&element->list, &data->list.list);
		++data->elements_count;
	} while (data->elements_count < elements_count);

	data->element_iter_current = NULL;
}

void free_mpu6050_data(struct mpu6050_data_holder *data_)
{
	struct mpu6050_data_list *node, *tmp;
	struct mpu6050_data_holder_internal *data =
		(struct mpu6050_data_holder_internal *)data_;

	if (!data)
		return;

	spin_lock(&data->lock);
	list_for_each_entry_safe(node, tmp, &data->list.list, list)	{
		pr_info("%s %s: freeing node %p",
			THIS_MODULE->name, __func__, node);
		list_del(&node->list);
		kfree(node);
	}
	spin_unlock(&data->lock);
}

void add_mpu6050_element(struct mpu6050_data_holder *data_,
	struct mpu6050_data_elements *element)
{
	int is_last = false;
	struct mpu6050_data_holder_internal *data =
		(struct mpu6050_data_holder_internal *)data_;

	if (!data || !element)
		return;

	spin_lock(&data->lock);
	if (!data->element_iter_current) {
		data->element_iter_current = list_first_entry(
			&data->list.list, struct mpu6050_data_list, list);
	} else {
		/* check if its the last element */
		is_last = list_is_last(
			&data->element_iter_current->list, &data->list.list);
		if (is_last) {
			struct mpu6050_data_list *first = list_first_entry(
				&data->list.list,
				struct mpu6050_data_list, list);
			if (first) {
				/* move first to the last and use it */
				list_move_tail(&first->list, &data->list.list);
				data->element_iter_current = list_next_entry(
					data->element_iter_current, list);
			}
		} else {
			data->element_iter_current = list_next_entry(
				data->element_iter_current, list);
		}
	}

	if (data->element_iter_current) {
		memcpy(data->element_iter_current->data.data, element->data,
			sizeof(data->element_iter_current->data.data));
		memcpy(data->element_iter_current->data.extra_data,
			element->extra_data,
			sizeof(data->element_iter_current->data.extra_data));
		/* pr_info("%s %s: was_last: %d next %p", THIS_MODULE->name, */
		/*	__func__, is_last, data->element_iter_current); */
	} else {
		/* pr_err("%s %s: data holder is not initialized properly", */
		/*	THIS_MODULE->name, __func__); */
	}
	spin_unlock(&data->lock);
}

bool get_active_element(struct mpu6050_data_holder *data_,
	struct mpu6050_data_elements *element)
{
	bool result = false;
	struct mpu6050_data_holder_internal *data =
		(struct mpu6050_data_holder_internal *)data_;

	if (!data || !element)
		return result;

	spin_lock(&data->lock);
	if (!data->element_iter_current)
		goto error1;
	*element = data->element_iter_current->data;
	result = true;

error1:
	spin_unlock(&data->lock);
	return result;
}

bool get_first_element(struct mpu6050_data_holder *data_,
	struct mpu6050_data_elements *element)
{
	struct mpu6050_data_holder_internal *data =
		(struct mpu6050_data_holder_internal *)data_;
	struct mpu6050_data_list *first = NULL;
	bool result = false;

	if (!data || !data->element_iter_current || !element)
		return result;

	spin_lock(&data->lock);
	if (!data->element_iter_current)
		goto error1;
	first = list_first_entry(
		&data->list.list, struct mpu6050_data_list, list);
	if (!first)
		goto error1;
	data->element_iter = first;
	*element = first->data;
	result = true;

error1:
	spin_unlock(&data->lock);
	return result;
}

bool get_next_element(struct mpu6050_data_holder *data_,
	struct mpu6050_data_elements *element)
{
	struct mpu6050_data_holder_internal *data =
		(struct mpu6050_data_holder_internal *)data_;
	struct mpu6050_data_list *next = NULL;
	bool result = false;

	if (!data || !element)
		return result;

	spin_lock(&data->lock);
	if (!data->element_iter_current || !data->element_iter)
		goto error1;

	if (data->element_iter == data->element_iter_current)
		next = data->element_iter;
	else
		next = list_next_entry(data->element_iter, list);

	if (!next)
		goto error1;

	data->element_iter = next;
	*element = next->data;
	result = true;

error1:
	spin_unlock(&data->lock);
	return result;
}

bool is_last_element(struct mpu6050_data_holder *data_)
{
	struct mpu6050_data_holder_internal *data =
		(struct mpu6050_data_holder_internal *)data_;
	bool result = true;

	if (!data)
		return result;

	spin_lock(&data->lock);
	if (!data->element_iter_current || !data->element_iter)
		goto error1;
	if (data->element_iter != data->element_iter_current)
		result = false;

error1:
	spin_unlock(&data->lock);
	return result;
}
